#include "nrf_sphero/sphero.hpp"
#include "nrf_sphero/sphero_scanner.hpp"
#include "nrf_sphero/utils/color.hpp"
#include <zephyr/kernel.h>
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

    Color red(255, 0, 0);

    // for (auto sphero : spheros) {
    //     sphero->set_matrix_fill(0, 0, 7, 7, red);
    // }

    LOG_INF("Done");

    return 0;
}