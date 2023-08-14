#include "io.hpp"
#include "../sphero.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(IO, LOG_LEVEL_DBG);

const void IO::fill_led_matrix(Sphero& sphero, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, RGBColor color, uint8_t tid)
{
    std::vector<unsigned char> data = { x1, y1, x2, y2, color.red, color.green, color.blue };

    auto packet = encode(*sphero.packet_manager, IO_DID, 62, tid, data);
    sphero.execute(packet);
}