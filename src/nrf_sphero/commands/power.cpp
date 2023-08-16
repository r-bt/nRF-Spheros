#include "power.hpp"
#include "../controls/packet.hpp"
#include "../sphero.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Power, LOG_LEVEL_DBG);

const Packet Power::wake(Sphero& sphero, uint8_t tid)
{
    auto packet = encode(*sphero.packet_manager, POWER_DID, 13, tid);
    return packet;
}