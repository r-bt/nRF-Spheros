#include "sphero.hpp"
#include "ble/scanner.h"
#include "ble/sphero_client.h"
#include "commands/power.hpp"
#include "commands/sensor.hpp"
#include "controls/processors.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Sphero, LOG_LEVEL_DBG);

Sphero::Sphero(uint8_t id)
{
    // Set the sphero client id
    sphero_id = id;

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

    LOG_INF("Sending packet");

    for (int i = 0; i < payload.size(); i++) {
        LOG_INF("%x", payload.at(i));
    }

    bt_sphero_client* sphero_client = scanner_get_sphero(sphero_id);

    if (sphero_client == nullptr) {
        LOG_ERR("Sphero not found");
        return;
    }

    bt_sphero_client_send(sphero_client, payload.data(), payload.size());

    scanner_release_sphero(sphero_client);
};

void Sphero::wake()
{
    LOG_DBG("Waking up sphero");
    Power::wake(*this);
}

void Sphero::set_locator_flags(bool locator_flags)
{
    Sensor::set_locator_flags(*this, locator_flags, static_cast<uint8_t>(Processors::SECONDARY));
}
