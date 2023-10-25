#include "io.hpp"
#include "../controls/packet.hpp"
#include "../sphero.hpp"
#include "../utils/utils.hpp"
#include <cstring>
#include <vector>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(IO, LOG_LEVEL_DBG);

const Packet IO::fill_led_matrix(Sphero& sphero, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, RGBColor color, uint8_t tid)
{
    std::vector<unsigned char> data = { x1, y1, x2, y2, color.red, color.green, color.blue };

    auto packet = encode(*sphero.packet_manager, IO_DID, 62, tid, data);
    return packet;
}

const Packet IO::set_led_matrix_color(Sphero& sphero, RGBColor color, uint8_t tid)
{
    std::vector<unsigned char> data = { color.red, color.green, color.blue };

    auto packet = encode(*sphero.packet_manager, IO_DID, 47, tid, data);
    return packet;
}

const Packet IO::set_led_matrix_pixel_color(Sphero& sphero, uint8_t x, uint8_t y, RGBColor color, uint8_t tid)
{
    std::vector<unsigned char> data = { x, y, color.red, color.green, color.blue };

    auto packet = encode(*sphero.packet_manager, IO_DID, 45, tid, data);
    return packet;
}

const Packet IO::set_all_leds_with_8_bit_mask(Sphero& sphero, uint8_t mask, std::vector<uint8_t> led_values, uint8_t tid)
{
    std::vector<uint8_t> data = int_to_bytes(mask, 1);

    for (uint8_t value : led_values) {
        data.push_back(value);
    }

    auto packet = encode(*sphero.packet_manager, IO_DID, 28, tid, data);
    return packet;
}

const Packet IO::set_led_matrix_character(Sphero& sphero, unsigned char str, RGBColor color, uint8_t tid)
{
    std::vector<unsigned char> data = { color.red, color.green, color.blue, str };

    auto packet = encode(*sphero.packet_manager, IO_DID, 66, tid, data);
    return packet;
}

const Packet IO::save_compressed_frame(Sphero& sphero, uint8_t index, std::vector<uint8_t> frame, uint8_t tid)
{
    std::vector<uint8_t> data = int_to_bytes(index, 2);

    for (uint8_t value : frame) {
        data.push_back(value);
    }

    auto packet = encode(*sphero.packet_manager, IO_DID, 48, tid, data);

    return packet;
}

const Packet IO::save_compressed_frame_animation(Sphero& sphero, uint8_t animation_id, uint8_t fps, bool fade_animation, std::vector<RGBColor> palette, std::vector<uint16_t> frame_indexes, uint8_t tid)
{
    std::vector<uint8_t> data = { animation_id };
    data.push_back(fps % 31);
    data.push_back(fade_animation);

    data.push_back(palette.size());

    for (RGBColor color : palette) {
        data.push_back(color.red);
        data.push_back(color.green);
        data.push_back(color.blue);
    }

    // Pack the frame indexes

    uint16_t frame_count = static_cast<uint16_t>(frame_indexes.size());

    data.push_back(static_cast<uint8_t>(frame_count >> 8)); // Push the high byte of frame count
    data.push_back(static_cast<uint8_t>(frame_count & 0xFF)); // Push the low byte of frame count

    for (uint16_t index : frame_indexes) {
        data.push_back(static_cast<uint8_t>(index >> 8)); // Push the high byte
        data.push_back(static_cast<uint8_t>(index & 0xFF)); // Push the low byte
    }

    auto packet = encode(*sphero.packet_manager, IO_DID, 49, tid, data);

    return packet;
}

const Packet IO::play_animation(Sphero& sphero, uint8_t animation_id, bool loop, uint8_t tid)
{
    std::vector<uint8_t> data = { animation_id, loop };

    auto packet = encode(*sphero.packet_manager, IO_DID, 67, tid, data);

    return packet;
}

const Packet IO::clear_matrix(Sphero& sphero, uint8_t tid)
{
    auto packet = encode(*sphero.packet_manager, IO_DID, 56, tid);

    return packet;
}