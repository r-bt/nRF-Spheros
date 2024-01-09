#include "nrf_sphero/sphero.hpp"
#include "nrf_sphero/sphero_scanner.hpp"
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
// Logging
#include <cstdlib>
#include <zephyr/logging/log.h>
#include <zephyr/timing/timing.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

// UART variables

#define RECIEVE_BUFF_SIZE 93
#define UART_RX_TIMEOUT 100
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)

static struct k_work_delayable uart_work;

const struct device* uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

struct uart_data_t {
    void* fifo_reserved;
    uint8_t data[RECIEVE_BUFF_SIZE];
    uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_rx_data);
static K_FIFO_DEFINE(fifo_uart_tx_data);

// DEFINE STATE MACHINE

enum class States {
    IDLE = 0,
    MATCH = 1,
    SET_COLORS = 2,
};

States state = States::IDLE;
uint8_t matching = 0;

static void uart_cb(const struct device* dev, struct uart_event* evt, void* user_data)
{
    ARG_UNUSED(dev);

    static size_t aborted_len;
    struct uart_data_t* buf;
    static uint8_t* aborted_buf;
    static bool disable_req;

    switch (evt->type) {
    case UART_TX_DONE:
        if ((evt->data.tx.len == 0) || (!evt->data.tx.buf)) {
            return;
        }

        if (aborted_buf) {
            buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
                data);
            aborted_buf = NULL;
            aborted_len = 0;
        } else {
            buf = CONTAINER_OF(evt->data.tx.buf,
                struct uart_data_t,
                data);
        }

        k_free(buf);

        buf = (uart_data_t*)k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
        if (!buf) {
            return;
        }

        if (uart_tx(uart, buf->data, buf->len, SYS_FOREVER_MS)) {
            LOG_WRN("Failed to send data over UART");
        }

        break;

    case UART_RX_RDY:
        buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data);
        buf->len += evt->data.rx.len;

        if (disable_req) {
            return;
        }

        if ((evt->data.rx.buf[buf->len - 1] == '\n')) {
            disable_req = true;
            uart_rx_disable(uart);
        }

        break;

    case UART_RX_DISABLED:
        disable_req = false;

        buf = (uart_data_t*)k_malloc(sizeof(*buf));
        if (buf) {
            buf->len = 0;
        } else {
            LOG_WRN("Not able to allocate UART receive buffer");
            k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
            return;
        }

        uart_rx_enable(uart, buf->data, sizeof(buf->data),
            UART_RX_TIMEOUT);

        break;

    case UART_RX_BUF_REQUEST:
        buf = (uart_data_t*)k_malloc(sizeof(*buf));
        if (buf) {
            buf->len = 0;
            uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
        } else {
            LOG_WRN("Not able to allocate UART receive buffer");
        }

        break;

    case UART_RX_BUF_RELEASED:
        buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t,
            data);

        if (buf->len > 0) {
            LOG_DBG("Adding %d bytes to fifo", buf->len);
            k_fifo_put(&fifo_uart_rx_data, buf);
        } else {
            k_free(buf);
        }

        break;

    case UART_TX_ABORTED:
        if (!aborted_buf) {
            aborted_buf = (uint8_t*)evt->data.tx.buf;
        }

        aborted_len += evt->data.tx.len;
        buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
            data);

        uart_tx(uart, &buf->data[aborted_len],
            buf->len - aborted_len, SYS_FOREVER_MS);

        break;

    default:
        break;
    }
}

static void uart_work_handler(struct k_work* item)
{
    struct uart_data_t* buf;

    buf = (uart_data_t*)k_malloc(sizeof(*buf));

    if (buf) {
        buf->len = 0;
    } else {
        LOG_WRN("Not able to allocate UART receive buffer (uart_work_handler)");
        k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
        return;
    }

    uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_RX_TIMEOUT);
}

const struct uart_config uart_cfg = {
    .baudrate = 115200,
    .parity = UART_CFG_PARITY_NONE,
    .stop_bits = UART_CFG_STOP_BITS_1,
    .data_bits = UART_CFG_DATA_BITS_8,
    .flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS
};

static int uart_init(void)
{
    int err;

    struct uart_data_t* rx;

    if (!device_is_ready(uart)) {
        return -ENODEV;
    }

    err = uart_configure(uart, &uart_cfg);

    if (err) {
        LOG_ERR("Cannot configure UART (err: %d)", err);
        return err;
    }

    // SETUP UART RX

    rx = (struct uart_data_t*)k_malloc(sizeof(*rx));
    if (rx) {
        rx->len = 0;
    } else {
        return -ENOMEM;
    }

    k_work_init_delayable(&uart_work, uart_work_handler);

    err = uart_callback_set(uart, uart_cb, NULL);
    if (err) {
        LOG_ERR("Cannot initalize UART callback");
        return err;
    }

    err = uart_rx_enable(uart, rx->data, sizeof(rx->data), UART_RX_TIMEOUT);
    if (err) {
        LOG_ERR("Cannot enable RX (err: %d)", err);
        k_free(rx);
    }

    return err;
}

bool validate_uart_packet(uart_data_t* rx)
{
    // Check for starting byte
    if (rx->data[0] != 0x8d) {
        LOG_ERR("Recieved invalid starting byte: 0x%02x", rx->data[0]);
        return false;
    }

    // Check for ending byte
    if (rx->data[rx->len - 1] != 0x0a) {
        LOG_ERR("Recieved invalid ending byte: 0x%02x", rx->data[rx->len - 1]);
        return false;
    }

    return true;
}

// Function to convert a uint8_t to States enum, with error handling
States uint8ToState(uint8_t value)
{
    if (value >= static_cast<uint8_t>(States::IDLE) && value <= static_cast<uint8_t>(States::SET_COLORS)) {
        return static_cast<States>(value);
    } else {
        return States::IDLE;
    }
}

void handle_idle_state(uart_data_t* rx)
{
    if (rx->len != 4) {
        LOG_ERR("Recieved %d bytes, expected 4", rx->len);
        return;
    }

    if (rx->data[1] != 0x01) {
        LOG_ERR("Recieved invalid command byte: 0x%02x", rx->data[1]);
        return;
    }

    state = uint8ToState(rx->data[2]);

    LOG_DBG("State is now %d", static_cast<uint8_t>(state));
}

uint8_t matching_index = 0;

std::vector<std::vector<uint8_t>> frame = {
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
};

std::vector<RGBColor> palette = { RGBColor(0, 0, 0), RGBColor(255, 255, 255) };

void handle_match_state(uart_data_t* rx, std::vector<std::shared_ptr<Sphero>>* spheros)
{
    if (matching >= spheros->size()) {
        LOG_ERR("Matching is greater than number of spheros");
        return;
    }

    if (rx->len < 3) {
        LOG_ERR("Recieved %d bytes, expected at least 3", rx->len);
        return;
    }

    LOG_DBG("Match command is: 0x%02x", rx->data[1]);

    auto sphero = (*spheros)[matching_index];

    switch (rx->data[1]) {
    case 0x01: // Increment matching index
        LOG_DBG("incrementing matching index");
        matching_index++;
        sphero->clear_matrix();
        break;
    case 0x02: // Set matrix to all white
        LOG_DBG("Setting matrix to all white");
        sphero->set_matrix_color(RGBColor(255, 255, 255));
        break;
    case 0x03: // Switch to orientation
        LOG_DBG("Switching to orientation");

        sphero->register_matrix_animation({ frame }, palette, 10, false);
        k_msleep(500);
        sphero->play_animation(0);

        break;
    case 0x04: // Handle changing heading and resetting aim
        LOG_DBG("Correcting heading");

        uint16_t heading = (rx->data[2] << 8) | (rx->data[3]); // big-endian format
        LOG_DBG("Angle is: %d", heading);
        auto response = sphero->drive_with_response(0, heading);

        sphero->wait_for_response(response);
        k_msleep(600);

        sphero->reset_aim();
        LOG_DBG("Resetted aim!");
        break;
    }
}

void handle_color_state(uart_data_t* rx, std::vector<std::shared_ptr<Sphero>>* spheros)
{
    if (rx->len != 93) {
        LOG_ERR("Recieved %d bytes, expected 48", rx->len);
        return;
    }

    if (rx->data[1] != 0x01) {
        LOG_ERR("Recieved invalid command byte: 0x%02x", rx->data[1]);
        return;
    }

    // Set the state colors
    for (int i = 0; i < spheros->size(); i++) {
        auto color = RGBColor(rx->data[(i * 3) + 2], rx->data[(i * 3 + 1) + 2], rx->data[(i * 3 + 2) + 2]);
        (*spheros)[i]->set_matrix_color(color);
    }

    // Set the velocities
    for (int i = 0; i < spheros->size(); i++) {
        uint8_t speed = rx->data[47 + (i * 3)];
        uint16_t heading = (rx->data[47 + (i * 3) + 1] << 8) | (rx->data[47 + (i * 3) + 2]); // big-endian format
        LOG_DBG("Speed is: %d, heading is: %d", speed, heading);
        (*spheros)[i]->drive(speed, heading);
    }
}

void reset(std::vector<std::shared_ptr<Sphero>>* spheros)
{
    LOG_DBG("Resetting state!");

    // Clear the LED matrix on all spheros
    for (auto sphero : *spheros) {
        sphero->clear_matrix();
        sphero->set_matrix_color(RGBColor(0, 0, 0));
    }

    matching_index = 0;
    state = States::IDLE;
}

void send_response(uint8_t* data, size_t data_size)
{
    struct uart_data_t* tx = (struct uart_data_t*)k_malloc(sizeof(*tx));

    tx->len = data_size + 2;

    tx->data[0] = 0x8d;
    memcpy(&tx->data[1], data, sizeof(data));
    tx->data[tx->len - 1] = 0x0a;

    int err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
    if (err) {
        LOG_ERR("Error when sending data to UART (err %d)", err);
        k_fifo_put(&fifo_uart_tx_data, tx);
    }
}

int main(void)
{
    int err;

    // SETUP SPHEROS

    SpheroScanner scanner;

    err = scanner.start_scanning();

    if (err) {
        LOG_ERR("Failed to start scanning (err %d)", err);
        return 0;
    } else {
        LOG_INF("Scanning started");
    }

    while (!scanner.found_all_spheros()) {
        k_msleep(5);
    }

    scanner.stop_scanning();

    LOG_DBG("Found all spheros");

    auto spheros = scanner.get_spheros();
    LOG_INF("Found %d spheros", scanner.get_num_spheros());

    // SETUP UART

    err = uart_init();

    if (err) {
        LOG_ERR("Failed to initialize UART (err %d)", err);
        return 1;
    }

    // Run the state machine

    LOG_DBG("Running state machine!");

    for (;;) {
        struct uart_data_t* rx = (struct uart_data_t*)k_fifo_get(&fifo_uart_rx_data, K_FOREVER);

        if (rx) {
            if (!validate_uart_packet(rx)) {
                k_free(rx);
                continue;
            }

            if (rx->data[1] == 0x00) {
                reset(&spheros);
            } else {
                switch (state) {
                case States::IDLE:
                    handle_idle_state(rx);
                    break;
                case States::MATCH:
                    handle_match_state(rx, &spheros);
                    break;
                case States::SET_COLORS:
                    handle_color_state(rx, &spheros);
                    break;
                }
            }

            LOG_DBG("Freeing rx!");

            k_free(rx);

            // Send response

            LOG_DBG("Sending response!");

            uint8_t data[] = { 0xFF };
            size_t data_size = sizeof(data) / sizeof(data[0]);
            send_response(data, data_size);

            LOG_DBG("Sent response!");
        }
    }

    return 0;
}