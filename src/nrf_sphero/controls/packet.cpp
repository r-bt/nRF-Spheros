#include "packet.hpp"
#include <numeric>
#include <vector>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Packet, LOG_LEVEL_DBG);

uint8_t packet_chk(std::vector<unsigned char> payload)
{
    return 0xff - (std::accumulate(payload.begin(), payload.end(), 0) & 0xff);
}

PacketFlags operator&(PacketFlags lhs, PacketFlags rhs)
{
    return static_cast<PacketFlags>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

PacketFlags operator|(PacketFlags lhs, PacketFlags rhs)
{
    return static_cast<PacketFlags>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

PacketFlags& operator|=(PacketFlags& lhs, PacketFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

std::vector<unsigned char> Packet::build() const
{
    std::vector<unsigned char> packet = { static_cast<unsigned char>(flags) };

    LOG_DBG("Flags: %d", static_cast<uint8_t>(flags));
    LOG_DBG("Did: %d", did);
    LOG_DBG("Cid: %d", cid);
    LOG_DBG("Seq: %d", seq);
    LOG_DBG("Tid: %d", tid);
    LOG_DBG("Sid: %d", sid);

    // Print data as hex
    for (int i = 0; i < data.size(); i++) {
        LOG_INF("%x", data.at(i));
    }

    if ((flags & PacketFlags::has_target_id) != PacketFlags::none) {
        packet.push_back(tid);
    }

    if ((flags & PacketFlags::has_source_id) != PacketFlags::none) {
        packet.push_back(sid);
    }

    packet.push_back(did);
    packet.push_back(cid);
    packet.push_back(seq);

    if ((flags & PacketFlags::is_response) != PacketFlags::none) {
        packet.push_back(static_cast<unsigned char>(err));
    }

    if (data.size() > 0) {
        packet.insert(packet.end(), data.begin(), data.end());
    }

    packet.push_back(packet_chk(packet));

    std::vector<unsigned char> escaped_packet = { static_cast<unsigned char>(PacketEncoding::start) };

    for (auto byte : packet) {

        switch (byte) {
        case static_cast<unsigned char>(PacketEncoding::start):
            escaped_packet.push_back(static_cast<unsigned char>(PacketEncoding::escape));
            escaped_packet.push_back(static_cast<unsigned char>(PacketEncoding::escaped_start));
            break;
        case static_cast<unsigned char>(PacketEncoding::end):
            escaped_packet.push_back(static_cast<unsigned char>(PacketEncoding::escape));
            escaped_packet.push_back(static_cast<unsigned char>(PacketEncoding::escaped_end));
            break;
        case static_cast<unsigned char>(PacketEncoding::escape):
            escaped_packet.push_back(static_cast<unsigned char>(PacketEncoding::escape));
            escaped_packet.push_back(static_cast<unsigned char>(PacketEncoding::escaped_escape));
            break;
        default:
            escaped_packet.push_back(byte);
            break;
        }
    }

    escaped_packet.push_back(static_cast<unsigned char>(PacketEncoding::end));

    return escaped_packet;
};