#include "drive.hpp"
#include "../controls/packet.hpp"
#include "../sphero.hpp"
#include "../utils/utils.hpp"
#include <vector>

const Packet Drive::drive(Sphero& sphero, uint8_t speed, uint16_t heading, DriveFlags flags, uint8_t tid)
{
    std::vector<unsigned char> data = { speed };

    auto bytes = int_to_bytes(heading, 2);
    data.insert(data.end(), bytes.begin(), bytes.end());

    data.push_back(static_cast<uint8_t>(flags));

    auto packet = encode(*sphero.packet_manager, DRIVE_DID, 7, tid, data);

    return packet;
}

const Packet Drive::reset_aim(Sphero& sphero, uint8_t tid)
{
    auto packet = encode(*sphero.packet_manager, DRIVE_DID, 6, tid);

    return packet;
}