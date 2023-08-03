#include "nrf_sphero/sphero_scanner.hpp"
// Logging
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void)
{
    LOG_INF("Starting");

    SpheroScanner scanner;

    LOG_INF("Started");

    scanner.start_scan();

    return 0;
}