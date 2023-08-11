#ifndef IO_H
#define IO_H

#include "../sphero.hpp"
#include "commands.hpp"

#define IO_DID 26

class IO : public Commands {
public:
    /**
     * @brief Fills given region of LED matrix a specific color
     *
     * @param[in] x1 The x coordinate of the first corner
     * @param[in] y1 The y coordinate of the first corner
     * @param[in] x2 The x coordinate of the second corner
     * @param[in] y2 The y coordinate of the second corner
     * @param[in] color The color to set matrix to
     * @param[in] tid The target id for the packet (optional)
     */
    static const void fill_led_matrix(Sphero& sphero, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, Color color, uint8_t tid = 0);
};

#endif // IO_H