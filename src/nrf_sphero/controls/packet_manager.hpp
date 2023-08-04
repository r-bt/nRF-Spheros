#ifndef PACKET_MANAGER_H
#define PACKET_MANAGER_H

#include "packet.hpp"
#include <vector>

class PacketManager {
private:
    uint8_t seq;

public:
    PacketManager();

    /**
     * @brief Create a new packet
     *
     * @returns Packet The newly created packet
     */
    Packet new_packet(uint8_t did, uint8_t cid, uint8_t tid = 0, std::vector<unsigned char>* data = nullptr);
};

#endif // PACKET_MANAGER_H