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
#define RECIEVE_TIMEOUT 200

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

    struct uart_data_t* buf;
    static bool disable_req;

    switch (evt->type) {
    case UART_RX_RDY:
        LOG_DBG("UART_RX_RDY");
        buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data);
        buf->len = evt->data.rx.len;

        if (disable_req) {
            return;
        }

        // We only handle one packet at a time

        disable_req = true;
        uart_rx_disable(uart);

        break;

    case UART_RX_DISABLED:
        LOG_DBG("UART_RX_DISABLED");
        disable_req = false;

        buf = (struct uart_data_t*)k_malloc(sizeof(*buf));
        if (buf) {
            buf->len = 0;
        } else {
            LOG_WRN("Not able to allocate UART receive buffer");
            return;
        }

        uart_rx_enable(uart, buf->data, sizeof(buf->data), RECIEVE_TIMEOUT);
        break;
    case UART_RX_BUF_REQUEST:
        LOG_DBG("UART_RX_BUF_RELEASED");
        buf = (struct uart_data_t*)k_malloc(sizeof(*buf));

        if (buf) {
            buf->len = 0;
            uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
        } else {
            LOG_WRN("Not able to allocate UART receive buffer");
        }

        break;
    case UART_RX_BUF_RELEASED:
        LOG_DBG("UART_RX_BUF_RELEASED");

        buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t, data);

        if (buf->len > 0) {
            k_fifo_put(&fifo_uart_rx_data, buf);
        } else {
            k_free(buf);
        }

        break;
    case UART_TX_ABORTED:
        LOG_DBG("UART_TX_ABORTED");
        break;
    default:
        break;
    }

    // struct state_callbacks* callbacks = (struct state_callbacks*)user_data;

    // switch (evt->type) {
    // case UART_RX_RDY:
    //     if (state == States::IDLE) {
    //         switch (evt->data.rx.buf[evt->data.rx.offset]) {
    //         case 1:
    //             state = States::MATCH;
    //             callbacks->match_callback();
    //             break;
    //         case 2:
    //             state = States::SET_COLORS;
    //             break;
    //         default:
    //             break;
    //         }
    //     } else if (state == States::MATCH) {
    //         switch (evt->data.rx.buf[evt->data.rx.offset]) {
    //         case 0:
    //             state == States::IDLE;
    //             callbacks->match_exit_callback();
    //             break;
    //         case 1:
    //             // Increment which one we're matching
    //             callbacks->match_callback();
    //             break;
    //         default:
    //             break;
    //         }
    //     } else if (state == States::SET_COLORS) {
    //         if (evt->data.rx.len == 1 && evt->data.rx.buf[evt->data.rx.offset] == 0) {
    //             state == States::IDLE;
    //         } else {
    //             callbacks->set_colors_callback(evt->data.rx);
    //         }
    //     }
    //     break;
    // case UART_RX_DISABLED:
    //     uart_rx_enable(dev, rx_buf, sizeof rx_buf, RECIEVE_TIMEOUT);
    //     break;
    // case UART_TX_DONE:
    //     break;
    // case UART_TX_ABORTED:
    //     LOG_ERR("Sending aborted");
    //     break;
    // default:
    //     break;
    // }
}

static int uart_init(void)
{
    int err;
    int pos;

    struct uart_data_t* rx;
    struct uart_data_t* tx;

    if (!device_is_ready(uart)) {
        return -ENODEV;
    }

    rx = (struct uart_data_t*)k_malloc(sizeof(*rx));

    if (!rx) {
        return -ENOMEM;
    }

    rx->len = 0;

    err = uart_callback_set(uart, uart_cb, NULL);

    if (err) {
        k_free(rx);
        LOG_ERR("Cannot initalize UART callback");
        return err;
    }

    tx = (struct uart_data_t*)k_malloc(sizeof(*tx));

    if (tx) {
        pos = snprintf((char*)tx->data, sizeof(tx->data), "Starting swarmalator model\r\n");

        if ((pos < 0 || pos >= sizeof(tx->data))) {
            k_free(tx);
            k_free(rx);
            LOG_ERR("snprintf returned %d", pos);
            return -ENOMEM;
        }

        tx->len = pos;
    } else {
        k_free(rx);
        return -ENOMEM;
    }

    err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);

    if (err) {
        k_free(rx);
        k_free(tx);
        LOG_ERR("Cannot display welcome message (err: %d)", err);
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

    while (true) {
        struct uart_data_t* rx = (struct uart_data_t*)k_fifo_get(&fifo_uart_rx_data, K_NO_WAIT);

        if (rx) {
            LOG_DBG("Recieved %d bytes", rx->len);

            if (rx->len != 45) {
                LOG_ERR("Recieved %d bytes, expected 45", rx->len);
                k_free(rx);
                continue;
            }

            for (int i = 0; i < std::min(spheros.size(), static_cast<std::size_t>(rx->len / 3)); i++) {
                auto color = RGBColor(rx->data[i * 3], rx->data[i * 3 + 1], rx->data[i * 3 + 2]);
                LOG_DBG("Color is: (%d, %d, %d)", color.red, color.green, color.blue);
                // spheros[i]->set_matrix_color(color);
            }

            k_free(rx);

            uint8_t tx_buf[] = { 0x8d,  };

            int ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_MS);
            if (ret) {
                LOG_ERR("Error when sending data to UART (err %d)", ret);
                break;
            }
        }
    }

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

    return 0;
}