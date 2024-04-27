#include "sphero_scanner.hpp"
#include "ble/scanner.h"
#include "sphero.hpp"
#include <cstdint>
#include <cstring> // Include <cstring> for strcpy
#include <memory>
#include <vector>
#include <zephyr/kernel.h>

SpheroScanner::SpheroScanner(std::vector<std::string> names)
{
    char** c_names = new char*[names.size()];
    for (size_t i = 0; i < names.size(); i++) {
        c_names[i] = new char[names[i].size() + 1];
        std::strcpy(c_names[i], names[i].c_str());
    }

    scanner_init(c_names, names.size());

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

unsigned int SpheroScanner::get_num_spheros()
{
    return scanner_get_sphero_count();
}

std::vector<std::shared_ptr<Sphero>> SpheroScanner::get_spheros()
{
    unsigned int num_spheros = scanner_get_sphero_count();

    std::vector<std::shared_ptr<Sphero>> spheros;
    for (size_t i = 0; i < num_spheros; i++) {
        std::shared_ptr<Sphero> sphero = std::make_shared<Sphero>(i);

        spheros.push_back(sphero);
    }

    return spheros;
}
