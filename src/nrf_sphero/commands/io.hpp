#ifndef IO_H
#define IO_H

#include "../controls/packet.hpp"
#include "../sphero.hpp"
#include "commands.hpp"
#include <vector>

#define IO_DID 26

class IO : public Commands {
public:
    /**
     * @brief Fills given region of LED matrix a specific color
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] x1 The x coordinate of the first corner
     * @param[in] y1 The y coordinate of the first corner
     * @param[in] x2 The x coordinate of the second corner
     * @param[in] y2 The y coordinate of the second corner
     * @param[in] color The color to set matrix to
     * @param[in] tid The target id for the packet (optional)
     */
    static const Packet fill_led_matrix(Sphero& sphero, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, RGBColor color, uint8_t tid = 0);

    /**
     * @brief Set's Sphero BOLT's LED matrix to specified color
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] color The color to set matrix to
     * @param[in] tid The target id for the packet (optional)
     */
    static const Packet set_led_matrix_color(Sphero& sphero, RGBColor color, uint8_t tid = 0);

    /**
     * @brief Sets all the LEDs with a 8 bit mask
     *
     * @param mask The 8 bit mask to set the LEDs with
     */
    static const Packet set_all_leds_with_8_bit_mask(Sphero& sphero, uint8_t mask, std::vector<uint8_t> led_values, uint8_t tid = 0);
};

#endif // IO_H