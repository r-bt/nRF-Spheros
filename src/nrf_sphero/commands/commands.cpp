#include "commands.hpp"
#include <cstdint>

uint8_t nibble_to_byte(uint8_t high, uint8_t low)
{
    return (high << 4) | low;
}

const Packet Commands::encode(PacketManager& manager, uint8_t did, uint8_t cid, uint8_t tid, std::vector<unsigned char> data)
{
    if (tid != 0) {
        tid = nibble_to_byte(0x1, tid);
    }

    return manager.new_packet(did, cid, tid, data);
}
