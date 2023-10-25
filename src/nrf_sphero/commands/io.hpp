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
     * @brief Set indivudal pixel on Sphero BOLT's LED matrix to specified color
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] x The x coordinate of the pixel
     * @param[in] y The y coordinate of the pixel
     * @param[in] color The color to set the pixel to
     * @param[in] tid The target id for the packet (optional)
     */
    static const Packet set_led_matrix_pixel_color(Sphero& sphero, uint8_t x, uint8_t y, RGBColor color, uint8_t tid = 0);

    /**
     * @brief Display character on Sphero BOLT's LED matrix
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] char The character to display
     * @param[in] color The color to set the pixel to
     * @param[in] tid The target id for the packet (optional)
     */
    static const Packet set_led_matrix_character(Sphero& sphero, unsigned char str, RGBColor color, uint8_t tid = 0);

    /**
     * @brief Saves a compressed frame with a specified index
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] index The index of the frame
     * @param[in] frame The frame to save
     * @param[in] tid The target id for the packet (optional)
     */
    static const Packet save_compressed_frame(Sphero& sphero, uint8_t index, std::vector<uint8_t> frame, uint8_t tid = 0);

    /**
     * @brief Save an animation
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] animation_id The id of the animation
     * @param[in] fps The frame rate of the animation
     * @param[in] fade_animation Whether or not to fade between frames
     * @param[in] palette The palette of colors to use
     * @param[in] frame_indexes The indexes of frames in the animation
     * @param[in] tid The target id for the packet (optional)
     *
     */
    static const Packet save_compressed_frame_animation(Sphero& sphero, uint8_t animation_id, uint8_t fps, bool fade_animation, std::vector<RGBColor> palette, std::vector<uint16_t> frame_indexes, uint8_t tid = 0);

    /**
     * @brief Play an animation
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] animation_id The id of the animation
     * @param[in] loop Whether or not to loop the animation
     * @param[in] tid The target id for the packet (optional)
     */
    static const Packet play_animation(Sphero& sphero, uint8_t animation_id, bool loop = true, uint8_t tid = 0);

    /**
     * @brief Clear matrix of animations
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] tid The target id for the packet (optional)
     */
    static const Packet clear_matrix(Sphero& sphero, uint8_t tid = 0);

    /**
     * @brief Sets all the LEDs with a 8 bit mask
     *
     * @param mask The 8 bit mask to set the LEDs with
     */
    static const Packet set_all_leds_with_8_bit_mask(Sphero& sphero, uint8_t mask, std::vector<uint8_t> led_values, uint8_t tid = 0);
};

#endif // IO_H