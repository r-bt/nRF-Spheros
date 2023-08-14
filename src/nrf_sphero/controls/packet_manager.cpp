#include "packet_manager.hpp"
#include "packet.hpp"
#include <vector>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PacketManager, LOG_LEVEL_DBG);

PacketManager::PacketManager()
{
    seq = 0;
}

Packet PacketManager::new_packet(uint8_t did, uint8_t cid, uint8_t tid, std::vector<unsigned char> data)
{
    PacketFlags flags = PacketFlags::requests_response | PacketFlags::is_activity;
    uint8_t sid = 0;

    if (tid != 0) {
        flags |= PacketFlags::has_source_id | PacketFlags::has_target_id;
        sid = 0x1;
    }

    Packet packet(flags, did, cid, seq, tid, sid, PacketError::success, data);

    LOG_DBG("flags = %d, sid = %d, did = %d, cid = %d, seq = %d, tid = %d", static_cast<uint8_t>(packet.flags), packet.sid, packet.did, packet.cid, packet.seq, packet.tid);

    seq = (seq + 1) % 0xff;

    return packet;
}