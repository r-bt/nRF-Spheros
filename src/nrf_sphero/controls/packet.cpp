#include "packet.hpp"
#include <numeric>
#include <stdexcept>
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

Packet::Packet(PacketFlags flags, uint8_t did, uint8_t cid, uint8_t seq, uint8_t tid, uint8_t sid, PacketError err, std::vector<unsigned char> data)
    : flags(flags)
    , did(did)
    , cid(cid)
    , seq(seq)
    , tid(tid)
    , sid(sid)
    , err(err)
    , data(data) {};

std::vector<unsigned char> Packet::build() const
{
    std::vector<unsigned char> packet = { static_cast<unsigned char>(flags) };

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

std::vector<unsigned char> Packet::unescape_data(std::vector<unsigned char> data)
{
    std::vector<unsigned char> unescaped_data;

    for (size_t i = 0; i < data.size(); i++) {
        if (data[i] == static_cast<unsigned char>(PacketEncoding::escape)) {
            switch (data[i + 1]) {
            case static_cast<unsigned char>(PacketEncoding::escaped_start):
                unescaped_data.push_back(static_cast<unsigned char>(PacketEncoding::start));
                break;
            case static_cast<unsigned char>(PacketEncoding::escaped_end):
                unescaped_data.push_back(static_cast<unsigned char>(PacketEncoding::end));
                break;
            case static_cast<unsigned char>(PacketEncoding::escaped_escape):
                unescaped_data.push_back(static_cast<unsigned char>(PacketEncoding::escape));
                break;
            default:
                LOG_ERR("Invalid escape sequence: %d", data[i + 1]);
                return {};
            }
            i++;
        } else {
            unescaped_data.push_back(data[i]);
        }
    }

    return unescaped_data;
}

const Packet Packet::parse_response(std::vector<unsigned char> data)
{
    if (data.empty()) {
        LOG_ERR("Empty data");
        throw std::runtime_error("Empty data");
    }

    uint8_t sop = data[0];
    uint8_t eop = data[data.size() - 1];

    data.erase(data.begin());
    data.pop_back();

    if (sop != static_cast<uint8_t>(PacketEncoding::start)) {
        LOG_ERR("Invalid start of packet: %d", sop);
        throw std::runtime_error("Invalid start of packet");
    }

    if (eop != static_cast<uint8_t>(PacketEncoding::end)) {
        LOG_ERR("Invalid end of packet: %d", eop);
        throw std::runtime_error("Invalid end of packet");
    }

    auto unescaped_data = Packet::unescape_data(data);

    unsigned char checksum = unescaped_data[unescaped_data.size() - 1];
    unescaped_data.pop_back();

    if (checksum != packet_chk(unescaped_data)) {
        LOG_ERR("Invalid checksum: %d", checksum);
        throw std::runtime_error("Invalid checksum");
    }

    PacketFlags flags = static_cast<PacketFlags>(unescaped_data[0]);
    unescaped_data.erase(unescaped_data.begin());

    uint8_t tid = 0;
    if ((flags & PacketFlags::has_target_id) != PacketFlags::none) {
        tid = unescaped_data[0];
        unescaped_data.erase(unescaped_data.begin());
    }

    uint8_t sid = 0;
    if ((flags & PacketFlags::has_source_id) != PacketFlags::none) {
        sid = unescaped_data[0];
        unescaped_data.erase(unescaped_data.begin());
    }

    uint8_t did = unescaped_data[0];
    uint8_t cid = unescaped_data[1];
    uint8_t seq = unescaped_data[2];

    PacketError err = PacketError::success;

    if ((flags & PacketFlags::is_response) != PacketFlags::none) {
        err = static_cast<PacketError>(unescaped_data[3]);
        unescaped_data.erase(unescaped_data.begin());
    }

    unescaped_data.erase(unescaped_data.begin(), unescaped_data.begin() + 3);

    return Packet(flags, tid, sid, did, cid, seq, err, unescaped_data);
}