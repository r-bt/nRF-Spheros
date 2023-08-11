#ifndef SPHERO_H
#define SPHERO_H

#include "ble/sphero_client.h"
#include "controls/packet.hpp"
#include "controls/packet_manager.hpp"

#define PACKET_PROCESSING_QUEUE_PRIORITY 4

class Sphero {
private:
    uint8_t sphero_id;

public:
    PacketManager* packet_manager;

    Sphero(uint8_t id);
    // Sphero(bt_sphero_client* client);
    ~Sphero();

    /**
     * @brief Execute a command
     *
     * @note Differs from Sphero v2 since doesn't add to a queue
     */
    void execute(const Packet& packet);

    /**
     * @brief Wake up Sphero from soft sleep. Nothing to do if awake.
     */
    void wake();

    /**
     * @brief Sets flags for the locator module.
     *
     * @param locator_flags The flags to set
     */
    void set_locator_flags(bool locator_flags);
};

#endif // SPHERO_H
