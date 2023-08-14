#include "sphero.hpp"
#include "ble/scanner.h"
#include "ble/sphero_client.h"
#include "controls/packet_collector.hpp"
#include "controls/processors.hpp"
#include "utils/color.hpp"
#include <functional>
#include <vector>
#include <zephyr/logging/log.h>
// COMMANDS
#include "commands/io.hpp"
#include "commands/power.hpp"
#include "commands/sensor.hpp"

LOG_MODULE_REGISTER(Sphero, LOG_LEVEL_DBG);

uint8_t Sphero::received_cb_wrapper(struct bt_sphero_client* sphero, const uint8_t* data, uint16_t len, void* context)
{

    Sphero* sphero_instance = static_cast<Sphero*>(context);

    sphero_instance->packet_collector->add_packet(data, len);

    return 1;
}

void Sphero::handle_packet(Packet packet)
{
    LOG_DBG("Handling packet, has error code %d", static_cast<uint8_t>(packet.err));

    // auto data = packet.build();

    // for (size_t i = 0; i < data.size(); i++) {
    //     LOG_DBG("%x", data.at(i));
    // }

    // Print data as hex
    // for (size_t i = 0; i < packet.data.size(); i++) {
    //     LOG_INF("%x", packet.data.at(i));
    // }
}

void Sphero::subscribe()
{
    bt_sphero_client* sphero_client = scanner_get_sphero(sphero_id);

    if (sphero_client == nullptr) {
        LOG_ERR("Sphero not found");
        return;
    }

    int err = bt_sphero_subscribe(sphero_client, &received_cb_wrapper, this);
    if (err) {
        LOG_ERR("Failed to subscribe to notifications (err %d)", err);
    }

    scanner_release_sphero(sphero_client);
}

Sphero::Sphero(uint8_t id)
{
    sphero_id = id;

    packet_manager = new PacketManager();

    packet_collector = new PacketCollector(std::bind(&Sphero::handle_packet, this, std::placeholders::_1));

    subscribe();

    wake();

    // set_locator_flags(false);
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

    for (size_t i = 0; i < payload.size(); i++) {
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
    Power::wake(*this);
}

void Sphero::set_locator_flags(bool locator_flags)
{
    Sensor::set_locator_flags(*this, locator_flags, static_cast<uint8_t>(Processors::SECONDARY));
}

void Sphero::set_matrix_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, RGBColor color)
{
    IO::fill_led_matrix(*this, x1, y1, x2, y2, color, static_cast<uint8_t>(Processors::SECONDARY));
}
