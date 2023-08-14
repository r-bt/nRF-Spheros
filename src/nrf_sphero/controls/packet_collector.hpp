#ifndef PACKET_COLLECTOR_H
#define PACKET_COLLECTOR_H

#include "packet.hpp"
#include <cstdint>
#include <functional>
#include <vector>
#include <zephyr/kernel.h>

class PacketCollector {
public:
    using PacketCollectorCallbackType = std::function<void(Packet packet)>;

    PacketCollector(PacketCollectorCallbackType cb);

    /**
     * @brief Add a packet to the collector
     *
     * @param data The data to add
     * @param len The length of the data
     */
    void add_packet(const uint8_t* data, uint16_t len);

private:
    std::vector<uint8_t> received_data;
    PacketCollectorCallbackType callback;
    struct k_mutex mutex; // Mutex for synchronization
    int count = 0;
};

#endif // PACKET_COLLECTOR_H