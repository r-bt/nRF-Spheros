#ifndef POWER_H
#define POWER_H

#include "../controls/packet.hpp"
#include "../sphero.hpp"
#include "commands.hpp"

#define POWER_DID 19

class Power : public Commands {
public:
    /**
     * @brief Wake up the sphero
     *
     * @param sphero The sphero to wake up
     * @param tid The target id for the packet (optional)
     */
    static const Packet wake(Sphero& sphero, uint8_t tid = 0);
};

#endif // POWER_H