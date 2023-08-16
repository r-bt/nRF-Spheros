#ifndef SENSOR_H
#define SENSOR_H

#include "../controls/packet.hpp"
#include "../sphero.hpp"
#include "commands.hpp"

#define SENSOR_DID 24

class Sensor : public Commands {
public:
    /**
     * @brief Sets flags for the locator module.
     *
     * @param sphero The sphero to act on
     * @param locator_flags The flags to set
     * @param tid The target id for the packet (optional)
     */
    static const Packet set_locator_flags(Sphero& sphero, bool locator_flags, uint8_t tid = 0);
};

#endif // SENSOR_H