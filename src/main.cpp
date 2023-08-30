#include "nrf_sphero/sphero.hpp"
#include "nrf_sphero/sphero_scanner.hpp"
// #include "nrf_sphero/utils/color.hpp"
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
// #include <zephyr/random/rand32.h>
// Logging
#include <algorithm> // std::min
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

// UART variables

#define RECIEVE_BUFF_SIZE 45
#define RECIEVE_TIMEOUT 250
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)
#define UART_TX_TIMEOUT 1000000

static struct k_work_delayable uart_work;

const struct device* uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

struct uart_data_t {
    void* fifo_reserved;
    uint8_t data[RECIEVE_BUFF_SIZE];
    uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_rx_data);

// DEFINE STATE MACHINE

enum class States {
    IDLE = 0,
    MATCH = 1,
    SET_COLORS = 2,
};

States state = States::SET_COLORS;
uint8_t matching = 0;

static void uart_cb(const struct device* dev, struct uart_event* evt, void* user_data)
{
    ARG_UNUSED(dev);

    static uint8_t* current_buf;
    static size_t aborted_len;
    static bool buf_release;
    struct uart_data_t* buf;
    static uint8_t* aborted_buf;

    switch (evt->type) {
    case UART_TX_DONE:
        if ((evt->data.tx.len == 0) || (!evt->data.tx.buf)) {
            return;
        }

        if (aborted_buf) {
            buf = CONTAINER_OF(aborted_buf, struct uart_data_t, data);

            aborted_buf = NULL;
            aborted_len = 0;
        } else {
            buf = CONTAINER_OF(evt->data.tx.buf, struct uart_data_t, data);
        }

        k_free(buf);

        break;
    case UART_RX_RDY:
        buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data);
        buf->len += evt->data.rx.len;
        buf_release = false;

        if (buf->len == RECIEVE_BUFF_SIZE) {
            k_fifo_put(&fifo_uart_rx_data, buf);
        } else if ((evt->data.rx.buf[buf->len - 1] == '\n')) {
            k_fifo_put(&fifo_uart_rx_data, buf);
            current_buf = evt->data.rx.buf;
            buf_release = true;
            uart_rx_disable(uart);
        }

        break;
    case UART_RX_DISABLED:
        buf = (uart_data_t*)k_malloc(sizeof(buf));
        if (buf) {
            buf->len = 0;
        } else {
            LOG_WRN("Not able to allocate UART receive buffer (UART_RX_DISABLED)");
            k_work_schedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
            return;
        }

        uart_rx_enable(uart, buf->data, sizeof(buf->data), RECIEVE_TIMEOUT);
        break;
    case UART_RX_BUF_REQUEST:
        buf = (uart_data_t*)k_malloc(sizeof(*buf));
        if (buf) {
            buf->len = 0;
            uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
        } else {
            LOG_WRN("Not able to allocate UART receive buffer (UART_RX_BUF_REQUEST)");
        }

        break;
    case UART_RX_BUF_RELEASED:
        buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t, data);

        if (buf_release && (current_buf != evt->data.rx_buf.buf)) {
            k_free(buf);
            buf_release = false;
            current_buf = NULL;
        }

        break;

    case UART_TX_ABORTED:
        if (!aborted_buf) {
            aborted_buf = (uint8_t*)evt->data.tx.buf;
        }

        aborted_len += evt->data.tx.len;
        buf = CONTAINER_OF(aborted_buf, struct uart_data_t, data);

        uart_tx(uart, &buf->data[aborted_len], buf->len - aborted_len, SYS_FOREVER_MS);

        break;
    default:
        break;
    }
}

static void uart_work_handler(struct k_work* item)
{
    struct uart_data_t* buf;

    LOG_DBG("Going to make the buffer");
    buf = (uart_data_t*)k_malloc(sizeof(*buf));
    LOG_DBG("Made the buffer!");

    if (buf) {
        buf->len = 0;
    } else {
        LOG_WRN("Not able to allocate UART receive buffer (uart_work_handler)");
        k_work_schedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
        return;
    }

    uart_rx_enable(uart, buf->data, sizeof(buf->data), RECIEVE_TIMEOUT);
}

static int uart_init(void)
{
    int err;

    struct uart_data_t* rx;

    if (!device_is_ready(uart)) {
        return -ENODEV;
    }

    rx = (struct uart_data_t*)k_malloc(sizeof(*rx));
    if (rx) {
        rx->len = 0;
    } else {
        return -ENOMEM;
    }

    err = uart_callback_set(uart, uart_cb, NULL);

    if (err) {
        LOG_ERR("Cannot initalize UART callback");
        return err;
    }

    err = uart_rx_enable(uart, rx->data, sizeof(rx->data), RECIEVE_TIMEOUT);

    if (err) {
        LOG_ERR("Cannot enable RX (err: %d)", err);
        k_free(rx);
    }

    return err;
}

void matched(const std::vector<std::shared_ptr<Sphero>>& spheros)
{

    LOG_DBG("Matching %d", matching);

    if (matching >= spheros.size()) {
        LOG_ERR("Matching is greater than number of spheros");
        return;
    }

    auto sphero = spheros[matching];

    sphero->set_matrix_color(RGBColor(255, 0, 0));

    if (matching > 0) {
        spheros[matching - 1]->set_matrix_color(RGBColor(0, 0, 0));
    }

    matching++;

    uint8_t tx_buf[] = { 0x8d, static_cast<uint8_t>(spheros.size() - matching), 0x0A };

    LOG_DBG("Sending %d", (spheros.size() - matching));

    int ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_MS);
    if (ret) {
        LOG_ERR("Error when sending data to UART (err %d)", ret);
        return;
    }
}

void set_colors(const std::vector<std::shared_ptr<Sphero>>& spheros, uart_event_rx evt)
{
    // LOG_DBG("Setting colors");
    // // Print the received data
    // LOG_DBG("Recieved %d bytes", evt.len);
    LOG_DBG("Recieved %d bytes", evt.len);

    for (int i = 0; i < evt.len; i++) {
        LOG_DBG("Hex: 0x%02x", evt.buf[evt.offset + i]);
    }

    LOG_DBG("Setting colors");
    // for (int i = 0; i < sizeof(buf); i++) {
    //     LOG_DBG("0x%02x", buf[i]);
    // }
    for (int i = 0; i < std::min(spheros.size(), evt.len / 3); i++) {
        auto color = RGBColor(evt.buf[evt.offset + (i * 3)], evt.buf[evt.offset + (i * 3 + 1)], evt.buf[evt.offset + (i * 3 + 2)]);
        LOG_DBG("Color is: (%d, %d, %d)", color.red, color.green, color.blue);
        // spheros[i]->set_matrix_color(color);
    }
};

int main(void)
{
    // SETUP UART

    int err;

    err = uart_init();

    if (err) {
        LOG_ERR("Failed to initialize UART (err %d)", err);
        return 1;
    }

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

    // Run the state machine

    // while (true) {
    //     struct uart_data_t* rx = (struct uart_data_t*)k_fifo_get(&fifo_uart_rx_data, K_NO_WAIT);

    //     if (rx) {
    //         LOG_DBG("Recieved %d bytes", rx->len);

    //         if (rx->len != 45) {
    //             LOG_ERR("Recieved %d bytes, expected 45", rx->len);
    //             k_free(rx);
    //             continue;
    //         }

    //         for (int i = 0; i < std::min(spheros.size(), static_cast<std::size_t>(rx->len / 3)); i++) {
    //             auto color = RGBColor(rx->data[i * 3], rx->data[i * 3 + 1], rx->data[i * 3 + 2]);
    //             LOG_DBG("Color is: (%d, %d, %d)", color.red, color.green, color.blue);
    //             // spheros[i]->set_matrix_color(color);
    //         }

    //         k_free(rx);

    //         uint8_t tx_buf[] = { 0x8d,  };

    //         int ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_MS);
    //         if (ret) {
    //             LOG_ERR("Error when sending data to UART (err %d)", ret);
    //             break;
    //         }
    //     }
    // }

    struct uart_data_t* tx;

    for (;;) {
        struct uart_data_t* buf;
        buf = (uart_data_t*)k_fifo_get(&fifo_uart_rx_data, K_FOREVER);

        if (buf) {
            for (int i = 0; i < std::min(spheros.size(), static_cast<std::size_t>(buf->len / 3)); i++) {
                auto color = RGBColor(buf->data[i * 3], buf->data[i * 3 + 1], buf->data[i * 3 + 2]);
                // LOG_DBG("Color is: (%d, %d, %d)", color.red, color.green, color.blue);
                spheros[i]->set_matrix_color(color);
            }

            k_free(buf);

            tx = (struct uart_data_t*)k_malloc(sizeof(*tx));

            uint8_t data[] = { 0x8d, 0x01, '\n' };

            tx->len = sizeof(data);
            memcpy(tx->data, data, sizeof(data));

            err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
            if (err) {
                LOG_ERR("Error when sending data to UART (err %d)", err);
                break;
            }

            continue;
        }

        k_free(buf);
    }

    return 0;

    // SETUP BLUETOOTH

    // LOG_INF("Starting");

    // // Setup UART with state callbacks

    // // struct state_callbacks callbacks = {
    // //     .match_callback = [&spheros]() { matched(spheros); },
    // //     .match_exit_callback = [&spheros]() {
    // //         if (matching > 0) {
    // //             spheros[matching - 1]->set_matrix_color(RGBColor(0,0,0));
    // //         }
    // //         matching = 0; },
    // //     .set_colors_callback = [&spheros](uart_event_rx evt) { set_colors(spheros, evt); },
    // // };

    // err = uart_callback_set(uart, uart_cb, NULL);
    // if (err) {
    //     return 1;
    // }

    // err = uart_rx_enable(uart, rx_buf, sizeof rx_buf, RECIEVE_TIMEOUT);
    // if (err) {
    //     return 1;
    // }

    // while (1) {
    //     k_msleep(100);
    // }

    // // uint16_t angle = 0;

    // // while (1) {

    // //     // HSVColor hsv(angle, 1, 1);

    // //     // angle += 6;

    // //     // LOG_DBG("RGB color is (%d, %d, %d), angle is %d", hsv.toRGB().red, hsv.toRGB().green, hsv.toRGB().blue, angle);

    // //     // // We don't want to exit the thread or else we'll lose the Sphero object which causes undefined behaviour
    // //     // for (auto sphero : spheros) {
    // //     //     sphero->set_matrix_fill(0, 0, 7, 7, hsv.toRGB());
    // //     // }
    // //     k_msleep(100);
    // // }
}