#include "sphero_scanner.hpp"
#include "ble/scanner.h"
#include "sphero.hpp"
#include <cstdint>
#include <vector>
#include <zephyr/kernel.h>

SpheroScanner::SpheroScanner()
{
    scanner_init();

    mInitialized = true;
}

SpheroScanner::~SpheroScanner()
{
    // NEED TO IMPLEMENT
    return;
}

int SpheroScanner::start_scanning()
{
    if (mInitialized) {
        return scanner_start();
    } else {
        return -1;
    }
}

int SpheroScanner::stop_scanning()
{
    if (mInitialized) {
        return scanner_stop();
    } else {
        return -1;
    }
}

bool SpheroScanner::found_all_spheros(uint64_t timeout)
{
    uint64_t current_time = k_uptime_get();
    if (current_time - last_sphero_found > timeout) {
        return true;
    }

    return false;
}

std::vector<Sphero> SpheroScanner::get_spheros()
{
    unsigned int num_spheros = scanner_get_sphero_count();

    std::vector<Sphero> spheros = {};
    for (size_t i = 0; i < num_spheros; i++) {
        bt_sphero_client* sphero_client = scanner_get_sphero(i);

        Sphero sphero(sphero_client);

        spheros.push_back(sphero);
    }

    return spheros;
}
