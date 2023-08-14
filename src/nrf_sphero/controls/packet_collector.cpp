#include "packet_collector.hpp"
#include "packet.hpp"
#include <vector>
#include <zephyr/debug/thread_analyzer.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PacketCollector, LOG_LEVEL_DBG);

PacketCollector::PacketCollector(PacketCollectorCallbackType cb)
{
    callback = cb;

    received_data = std::vector<uint8_t>();
}

void PacketCollector::add_packet(const uint8_t* data, uint16_t len)
{
    for (size_t i = 0; i < len; i++) {
        received_data.push_back(data[i]);
        if (data[i] == static_cast<uint8_t>(PacketEncoding::end)) {
            if (received_data.size() < 6) {
                LOG_ERR("Very small packet");
                return;
            }
            auto packet = Packet::parse_response(received_data);
            callback(packet);
            received_data.clear();
        }
    }
}
