#ifndef COMMANDS_H
#define COMMANDS_H

#include "../controls/packet.hpp"
#include "../controls/packet_manager.hpp"
#include <vector>

class Commands {
protected:
    static const Packet encode(PacketManager& manager, uint8_t did, uint8_t cid, uint8_t tid, std::vector<unsigned char> data = {});
};

#endif // COMMANDS_H