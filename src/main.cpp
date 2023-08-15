#include "nrf_sphero/sphero.hpp"
#include "nrf_sphero/sphero_scanner.hpp"
#include "nrf_sphero/utils/color.hpp"
#include <zephyr/kernel.h>
#include <zephyr/random/rand32.h>
// Logging
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void)
{
    LOG_INF("Starting");

    SpheroScanner scanner;

    LOG_INF("Started");

    int err;

    err = scanner.start_scanning();

    if (err) {
        LOG_ERR("Failed to start scanning (err %d)", err);
        return 0;
    } else {
        LOG_INF("Scanning started");
    }

    while (!scanner.found_all_spheros()) {
        // Do nothing
        k_msleep(5);
    }

    scanner.stop_scanning();

    LOG_INF("All spheros found!");

    auto spheros = scanner.get_spheros();

    // Print number of spheros
    LOG_INF("Found %d spheros", scanner.get_num_spheros());

    // Print amount of found spheros
    LOG_INF("%d spheros in std::vector", spheros.size());

    // TODO: Why do need to have this wait (otherwise the LED command isn't obeyed)
    k_msleep(1000);

    LOG_INF("Done");

    // Loop that prints the pointer location of the sphero instance
    // while (true) {
    //     for (auto sphero : spheros) {
    //         LOG_DBG("Pointer location is %p", sphero.get());
    //         // Sleep for 1 second
    //         k_msleep(1000);
    //     }
    // }

    uint16_t angle = 0;

    while (1) {

        // HSVColor hsv(angle, 1, 1);

        // angle += 6;

        // LOG_DBG("RGB color is (%d, %d, %d), angle is %d", hsv.toRGB().red, hsv.toRGB().green, hsv.toRGB().blue, angle);

        // // We don't want to exit the thread or else we'll lose the Sphero object which causes undefined behaviour
        // for (auto sphero : spheros) {
        //     sphero->set_matrix_fill(0, 0, 7, 7, hsv.toRGB());
        // }
        k_msleep(100);
    }

    return 0;
}