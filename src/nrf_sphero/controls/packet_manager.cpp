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
    Packet packet;

    packet.flags = PacketFlags::requests_response | PacketFlags::is_activity;
    packet.sid = 0;

    if (tid != 0) {
        packet.flags |= PacketFlags::has_source_id | PacketFlags::has_target_id;
        packet.sid = 0x1;
    }

    packet.did = did;
    packet.cid = cid;
    packet.seq = seq;
    packet.tid = tid;
    packet.data = data;

    LOG_DBG("flags = %d, sid = %d, did = %d, cid = %d, seq = %d, tid = %d", static_cast<uint8_t>(packet.flags), packet.sid, packet.did, packet.cid, packet.seq, packet.tid);

    seq = (seq + 1) % 0xff;

    return packet;
}