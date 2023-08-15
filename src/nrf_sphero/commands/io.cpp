#include "io.hpp"
#include "../sphero.hpp"
#include <cstring>
#include <vector>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(IO, LOG_LEVEL_DBG);

template <typename T>
std::vector<uint8_t> int_to_bytes(T i)
{
    std::vector<uint8_t> bytes(sizeof(T));
    std::memcpy(&bytes[0], &i, sizeof(T));
    return bytes;
}

const void IO::fill_led_matrix(Sphero& sphero, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, RGBColor color, uint8_t tid)
{
    std::vector<unsigned char> data = { x1, y1, x2, y2, color.red, color.green, color.blue };

    auto packet = encode(*sphero.packet_manager, IO_DID, 62, tid, data);
    sphero.execute(packet);
}

const void IO::set_led_matrix_color(Sphero& sphero, RGBColor color, uint8_t tid)
{
    std::vector<unsigned char> data = { color.red, color.green, color.blue };

    auto packet = encode(*sphero.packet_manager, IO_DID, 47, tid, data);
    sphero.execute(packet);
}

const void IO::set_all_leds_with_8_bit_mask(Sphero& sphero, uint8_t mask, std::vector<uint8_t> led_values, uint8_t tid)
{
    std::vector<uint8_t> data = int_to_bytes(mask);

    for (uint8_t value : led_values) {
        data.push_back(value);
    }

    auto packet = encode(*sphero.packet_manager, IO_DID, 28, tid, data);
    sphero.execute(packet);
}