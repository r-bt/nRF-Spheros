#include "sensor.hpp"
#include "../controls/packet.hpp"
#include "../sphero.hpp"
#include <vector>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Sensor, LOG_LEVEL_DBG);

const Packet Sensor::set_locator_flags(Sphero& sphero, bool locator_flags, uint8_t tid)
{
    std::vector<unsigned char> data = { static_cast<unsigned char>(locator_flags) };

    auto packet = encode(*sphero.packet_manager, SENSOR_DID, 23, tid, data);
    return packet;
}