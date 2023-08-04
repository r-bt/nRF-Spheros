#include "nrf_sphero/sphero.hpp"
#include "nrf_sphero/sphero_scanner.hpp"
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

    LOG_INF("Got spheros!");

    LOG_INF("Waking up the first sphero!");

    for (auto sphero : spheros) {
        sphero.wake();
    }

    return 0;
}