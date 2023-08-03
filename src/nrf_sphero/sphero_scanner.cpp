#include "sphero_scanner.hpp"
#include "ble/scanner.h"

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

int SpheroScanner::start_scan()
{
    if (mInitialized) {
        return scanner_start();
    } else {
        return -1;
    }
}
