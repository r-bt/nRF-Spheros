#ifndef COLOR_H
#define COLOR_H

#include <cstdint>

class RGBColor {
public:
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    /**
     * @brief Construct a new RGBColor object
     *
     * @param red The red value [0,255]
     * @param green The green value [0,255]
     * @param blue The blue value [0,255]
     */
    RGBColor(uint8_t red, uint8_t green, uint8_t blue);
};

class HSVColor {
public:
    uint8_t hue;
    uint8_t saturation;
    uint8_t value;

    /**
     * @brief Construct a new HSVColor object
     *
     * @param hue The hue of the color [0,360]
     * @param saturation The saturation of the color [0,1]
     * @param value The value of the color [0,1]
     */
    HSVColor(uint8_t hue, uint8_t saturation, uint8_t value);

    /**
     * @brief Convert HSV color to RGB color
     *
     * @return RGBColor The RGB color
     */
    RGBColor toRGB();
};

#endif // COLOR_H