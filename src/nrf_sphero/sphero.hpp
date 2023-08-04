#ifndef SPHERO_H
#define SPHERO_H

#include "ble/sphero_client.h"
#include "controls/packet.hpp"
#include "controls/packet_manager.hpp"

#define PACKET_PROCESSING_QUEUE_PRIORITY 4

class Sphero {
private:
    bt_sphero_client* sphero_client;

public:
    PacketManager* packet_manager;

    Sphero(bt_sphero_client* client);
    ~Sphero();

    /**
     * @brief Execute a command
     *
     * @note Differs from Sphero v2 since doesn't add to a queue
     */
    void execute(const Packet& packet);

    /**
     * @brief Wake up the Sphero
     */
    void wake();
};

#endif // SPHERO_H
