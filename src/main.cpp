#include "nrf_sphero/sphero.hpp"
#include "nrf_sphero/sphero_scanner.hpp"
#include "nrf_sphero/utils/color.hpp"

// #include "ble/ble_peripheral.h"
// #include "ble/swarmalator_service.h"

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
// #include <zephyr/random/rand32.h>
// Logging
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

// Swarmalator Service Packets

#define MAX_PACKET_SIZE 45

struct swarmalator_packet_t {
    void* fifo_reserved;
    uint8_t data[MAX_PACKET_SIZE];
    uint16_t len;
};

static K_FIFO_DEFINE(fifo_swamalator_packets);

// State machine

enum class States {
    IDLE = 0,
    MATCH = 1,
    SET_COLORS = 2,
};

States state = States::IDLE;
// uint8_t matching = 0;

// struct state_callbacks {
//     std::function<void()> match_callback;
//     std::function<void()> match_exit_callback;
//     std::function<void()> set_colors_callback;
// };

// static void uart_cb(const struct device* dev, struct uart_event* evt, void* user_data)
// {
//     struct state_callbacks* callbacks = (struct state_callbacks*)user_data;

//     switch (evt->type) {
//     case UART_RX_RDY:
//         if (state == States::IDLE) {
//             switch (evt->data.rx.buf[evt->data.rx.offset]) {
//             case 1:
//                 state = States::MATCH;
//                 callbacks->match_callback();
//                 break;
//             case 2:
//                 state = States::SET_COLORS;
//                 break;
//             default:
//                 break;
//             }
//         } else if (state == States::MATCH) {
//             switch (evt->data.rx.buf[evt->data.rx.offset]) {
//             case 0:
//                 state == States::IDLE;
//                 callbacks->match_exit_callback();
//                 break;
//             case 1:
//                 // Increment which one we're matching
//                 callbacks->match_callback();
//                 break;
//             default:
//                 break;
//             }
//         }
//         break;
//     case UART_RX_DISABLED:
//         uart_rx_enable(dev, rx_buf, sizeof rx_buf, RECIEVE_TIMEOUT);
//         break;
//     case UART_TX_DONE:
//         // Print the sent data
//         LOG_DBG("Sent %d bytes", evt->data.tx.len);
//         for (int i = 0; i < evt->data.tx.len; i++) {
//             LOG_DBG("0x%02x", evt->data.tx.buf[i]);
//         }
//         break;
//     case UART_TX_ABORTED:
//         LOG_ERR("Sending aborted");
//         break;
//     default:
//         break;
//     }
// }

// void matched(const std::vector<std::shared_ptr<Sphero>>& spheros)
// {

//     LOG_DBG("Matching %d", matching);

//     if (matching >= spheros.size()) {
//         LOG_ERR("Matching is greater than number of spheros");
//         return;
//     }

//     auto sphero = spheros[matching];

//     sphero->set_matrix_color(RGBColor(255, 0, 0));

//     if (matching > 0) {
//         spheros[matching - 1]->set_matrix_color(RGBColor(0, 0, 0));
//     }

//     matching++;

//     uint8_t tx_buf[] = { 0x8d, static_cast<uint8_t>(spheros.size() - matching), 0x0A };

//     LOG_DBG("Sending %d", (spheros.size() - matching));

//     int ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_MS);
//     if (ret) {
//         LOG_ERR("Error when sending data to UART (err %d)", ret);
//         return;
//     }
// }

void set_colors(const std::vector<std::shared_ptr<Sphero>>& spheros, swarmalator_packet_t* packet)
{
    for (int i = 0; i < std::min(spheros.size(), static_cast<size_t>(packet->len / 3)); i++) {
        auto color = RGBColor(packet->data[(i * 3)], packet->data[(i * 3 + 1)], packet->data[(i * 3 + 2)]);
        LOG_DBG("Color is: (%d, %d, %d)", color.red, color.green, color.blue);
        spheros[i]->set_matrix_color(color);
    }
};

void swarmalator_packet_cb(uint8_t* data, uint16_t len, void* context)
{

    struct swarmalator_packet_t* packet;
    packet = (struct swarmalator_packet_t*)k_malloc(sizeof(struct swarmalator_packet_t));

    memcpy(packet->data, data, len);
    packet->len = len;

    LOG_DBG("Putting into fifo");
    k_fifo_put(&fifo_swamalator_packets, packet);
}

// struct swarmalator_cb_struct swarmalator_cb {
//     .packet_cb = &swarmalator_packet_cb,
//     .packet_cb_context = NULL
// };

int main(void)
{
    LOG_DBG("Starting!");
    // SETUP UART

    int err;

    // err = bt_peripheral_init(&swarmalator_cb);

    // if (err) {
    //     LOG_ERR("Failed to initialize BLE peripheral (err %d)", err);
    //     return 1;
    // }

    // SETUP BLUETOOTH

    LOG_INF("Starting");

    SpheroScanner scanner;

    LOG_INF("Started");

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

    LOG_INF("Found all spheros");

    auto spheros = scanner.get_spheros();

    LOG_INF("Found %d spheros", scanner.get_num_spheros());

    // STATE MACHINE

    while (true) {
        struct swarmalator_packet_t* packet;
        packet = (struct swarmalator_packet_t*)k_fifo_get(&fifo_swamalator_packets, K_FOREVER);

        if (packet) {
            set_colors(spheros, packet);
        }

        k_free(packet);
    }

    // // Setup UART with state callbacks

    // struct state_callbacks callbacks = {
    //     .match_callback = [&spheros]() { matched(spheros); },
    //     .match_exit_callback = [&spheros]() {
    //         if (matching > 0) {
    //             spheros[matching - 1]->set_matrix_color(RGBColor(0,0,0));
    //         }
    //         matching = 0; },
    //     .set_colors_callback = &set_colors
    // };

    // err = uart_callback_set(uart, uart_cb, &callbacks);
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

    // uint16_t angle = 0;

    // while (1) {

    //     // HSVColor hsv(angle, 1, 1);

    //     // angle += 6;

    //     // LOG_DBG("RGB color is (%d, %d, %d), angle is %d", hsv.toRGB().red, hsv.toRGB().green, hsv.toRGB().blue, angle);

    //     // // We don't want to exit the thread or else we'll lose the Sphero object which causes undefined behaviour
    //     // for (auto sphero : spheros) {
    //     //     sphero->set_matrix_fill(0, 0, 7, 7, hsv.toRGB());
    //     // }
    //     k_msleep(100);
    // }

    return 0;
}