#include "sphero.hpp"
#include "ble/scanner.h"
#include "ble/sphero_client.h"
#include "controls/packet_collector.hpp"
#include "controls/processors.hpp"
#include "utils/color.hpp"
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <unordered_map>
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
    auto id = packet.id();

    if (waiting.find(id) == waiting.end()) {
        LOG_ERR("No signal found for packet id %d", id);
        return;
    }

    auto signal = waiting[id];

    responses.insert_or_assign(id, packet);

    k_poll_signal_raise(signal.get(), id);
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

    auto response = wake();

    wait_for_response(response);

    turn_off_all_leds();
};

Sphero::~Sphero()
{
    delete packet_manager;
};

CommandResponse Sphero::execute(const Packet& packet)
{
    auto payload = packet.build();

    LOG_INF("Sending packet");

    for (size_t i = 0; i < payload.size(); i++) {
        LOG_INF("%x", payload.at(i));
    }

    bt_sphero_client* sphero_client = scanner_get_sphero(sphero_id);

    if (sphero_client == nullptr) {
        LOG_ERR("Sphero not found");
        return nullptr;
    }

    bt_sphero_client_send(sphero_client, payload.data(), payload.size());

    scanner_release_sphero(sphero_client);

    /**
     * Create a signal and add it to the waiting map
     */

    std::shared_ptr<struct k_poll_signal> signal = std::make_shared<struct k_poll_signal>();

    k_poll_signal_init(signal.get());

    auto events = std::make_unique<k_poll_event[]>(1);
    events[0] = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, signal.get());

    auto id = packet.id();

    waiting.insert_or_assign(id, signal);

    return events;
};

CommandResponse Sphero::wake()
{
    auto packet = Power::wake(*this);

    return execute(packet);
}

CommandResponse Sphero::set_locator_flags(bool locator_flags)
{
    auto packet = Sensor::set_locator_flags(*this, locator_flags, static_cast<uint8_t>(Processors::SECONDARY));

    return execute(packet);
}

CommandResponse Sphero::set_matrix_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, RGBColor color)
{
    auto packet = IO::fill_led_matrix(*this, x1, y1, x2, y2, color, static_cast<uint8_t>(Processors::SECONDARY));

    return execute(packet);
}

CommandResponse Sphero::set_matrix_color(RGBColor color)
{
    auto packet = IO::set_led_matrix_color(*this, color, static_cast<uint8_t>(Processors::SECONDARY));

    return execute(packet);
}

CommandResponse Sphero::set_all_leds_with_8_bit_mask(uint8_t mask, std::vector<uint8_t> led_values)
{
    auto packet = IO::set_all_leds_with_8_bit_mask(*this, mask, led_values, static_cast<uint8_t>(Processors::PRIMARY));
    return execute(packet);
}

void Sphero::set_all_leds_with_map(std::unordered_map<Sphero::LEDs, uint8_t> mapping)
{
    uint8_t mask = 0;

    std::vector<uint8_t> led_values = {};

    for (int i = 0; i < static_cast<int>(Sphero::LEDs::LAST); ++i) {
        Sphero::LEDs led = static_cast<Sphero::LEDs>(i);
        if (mapping.find(led) != mapping.end()) {
            mask |= 1 << static_cast<uint8_t>(led);
            led_values.push_back(mapping[led]);
        }
    }

    if (mask != 0) {
        set_all_leds_with_8_bit_mask(mask, led_values);
    }
}

void Sphero::turn_off_all_leds()
{
    std::unordered_map<Sphero::LEDs, uint8_t> mapping;

    for (int i = 0; i < static_cast<int>(Sphero::LEDs::LAST); ++i) {
        mapping[static_cast<Sphero::LEDs>(i)] = 0;
    }

    set_all_leds_with_map(mapping);
    set_matrix_color(RGBColor(0, 0, 0));
}

std::optional<Packet> Sphero::wait_for_response(const CommandResponse& response)
{
    int err = 0;

    err = k_poll(response.get(), 1, K_MSEC(10000));

    if (err) {
        LOG_ERR("Failed to wait for response (err %d)", err);
        return std::nullopt;
    }

    auto res = response.get()->signal->result;

    auto packet = responses.find(res);

    if (packet == responses.end()) {
        LOG_ERR("No packet for packet id %d", res);
        return std::nullopt;
    }

    responses.erase(res);
    waiting.erase(res);

    return packet->second;
}