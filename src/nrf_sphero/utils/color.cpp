#include "color.hpp"
#include <cmath>

RGBColor::RGBColor(uint8_t red, uint8_t green, uint8_t blue)
    : red(red)
    , green(green)
    , blue(blue) {};

HSVColor::HSVColor(uint8_t hue, uint8_t saturation, uint8_t value)
{
    this->hue = hue % 360;
    this->saturation = saturation;
    this->value = value;
}

RGBColor HSVColor::toRGB()
{
    double h = static_cast<double>(hue) / 60.0;
    double s = static_cast<double>(saturation);
    double v = static_cast<double>(value);

    double c = v * s;
    double x = c * (1.0 - std::fabs(std::fmod(h, 2.0) - 1.0));
    double m = v - c;

    double r1, g1, b1;

    if (h >= 0.0 && h < 1.0) {
        r1 = c;
        g1 = x;
        b1 = 0.0;
    } else if (h >= 1.0 && h < 2.0) {
        r1 = x;
        g1 = c;
        b1 = 0.0;
    } else if (h >= 2.0 && h < 3.0) {
        r1 = 0.0;
        g1 = c;
        b1 = x;
    } else if (h >= 3.0 && h < 4.0) {
        r1 = 0.0;
        g1 = x;
        b1 = c;
    } else if (h >= 4.0 && h < 5.0) {
        r1 = x;
        g1 = 0.0;
        b1 = c;
    } else {
        r1 = c;
        g1 = 0.0;
        b1 = x;
    }

    uint8_t r = static_cast<uint8_t>((r1 + m) * 255);
    uint8_t g = static_cast<uint8_t>((g1 + m) * 255);
    uint8_t b = static_cast<uint8_t>((b1 + m) * 255);

    return RGBColor(r, g, b);
}
