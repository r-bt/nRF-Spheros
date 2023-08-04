#include "power.hpp"
#include "../sphero.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Power, LOG_LEVEL_DBG);

const void Power::wake(Sphero& sphero, uint8_t tid)
{
    LOG_DBG("Waking up sphero");
    auto packet = encode(*sphero.packet_manager, POWER_DID, 13, tid);
    sphero.execute(packet);
}