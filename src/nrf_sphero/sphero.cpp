#include "sphero.hpp"
#include "ble/sphero_client.h"
#include "commands/power.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Sphero, LOG_LEVEL_DBG);

Sphero::Sphero(bt_sphero_client* client)
{
    if (client == nullptr) {
        LOG_ERR("Client is null");
        return;
    }

    // Set the client
    sphero_client = client;

    // Create a unique instance of PacketManager
    packet_manager = new PacketManager();
};

Sphero::~Sphero()
{
    delete packet_manager;
};

void Sphero::execute(const Packet& packet)
{
    LOG_DBG("Executing packet");
    auto payload = packet.build();

    // // Log the packet
    // // LOG_INF("Sending packet: %s", payload.);
    LOG_INF("Sending packet");

    for (int i = 0; i < payload.size(); i++) {
        // std::cout << input.at(i) << ' ';
        LOG_INF("%x", payload.at(i));
    }

    bt_sphero_client_send(sphero_client, payload.data(), payload.size());
};

void Sphero::wake()
{
    LOG_DBG("Waking up sphero");
    Power::wake(*this);
}