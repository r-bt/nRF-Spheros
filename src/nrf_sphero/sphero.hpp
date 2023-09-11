#ifndef SPHERO_H
#define SPHERO_H

#include "ble/sphero_client.h"
#include "controls/packet.hpp"
#include "controls/packet_collector.hpp"
#include "controls/packet_manager.hpp"
#include "utils/color.hpp"
#include <memory>
#include <optional>
#include <unordered_map>

#define PACKET_PROCESSING_QUEUE_PRIORITY 4

typedef std::unique_ptr<k_poll_event[]> CommandResponse;

/**
 * This class specifically implements a Sphero BOLT
 * (as opposed to a generic Sphero which is then expanded on like in spherov2)
 *
 * However, some efforts have been made to make it simple in the future to add support for other Sphero models
 * (e.g. Sphero Mini, Sphero SPRK+, etc.)
 * */

class Sphero {
private:
    /** @brief The id of the Sphero Context */
    uint8_t sphero_id;

    /** @brief The index of the next animation frame */
    uint16_t frame_index;

    /** @brief The index of the next animation */
    uint8_t animation_index;

    /**
     * @brief Subscribe to notifications from the Sphero
     */
    void subscribe();

    /**
     * @brief Static wrapper for the handle_recevied_data function
     *
     * @note This is required since the callback function must be static to be passed to the C API
     */
    static bt_sphero_received_cb_t received_cb_wrapper;

    /**
     * @brief Tracks recieved packets and calls the callback function when a complete packet is received
     */
    PacketCollector* packet_collector;

    /**
     * @brief Handle a packet
     *
     * @note This function is called when a complete packet is received
     */
    void handle_packet(Packet packet);

    /**
     * @brief Handle setting up signals to wait for response
     */
    CommandResponse setup_response(const Packet& packet);

    /**
     * @brief Map of packet keys to queues of k_poll_signals which are used to pass packet response back
     * to executing function
     */
    std::unordered_map<uint32_t, std::shared_ptr<struct k_poll_signal>> waiting;

    /**
     * @brief Map of packet keys to packets which are used to pass packet response back
     */
    std::unordered_map<uint32_t, Packet> responses;

    /**
     * @brief Creates packet to tell Sphero to drive with speed in heading
     *
     * @param[in] speed The speed to drive at
     * @param[in] heading The heading to drive at
     *
     * @retval
     */
    Packet get_drive_packet(uint8_t speed, uint16_t heading);

public:
    PacketManager* packet_manager;

    Sphero(uint8_t id);

    ~Sphero();

    /**
     * @brief Available LEDs on Sphero Bolt
     */
    enum class LEDs : uint8_t {
        FRONT_RED,
        FRONT_GREEN,
        FRONT_BLUE,
        BACK_RED,
        BACK_GREEN,
        BACK_BLUE,
        LAST
    };

    /**
     * @brief Execute a command
     *
     * @note Differs from Sphero v2 since doesn't add to a queue
     */
    void execute(const Packet& packet);

    /**
     * @brief Wake up Sphero from soft sleep. Nothing to do if awake.
     */
    void wake();

    /**
     * @brief Wake up Sphero from soft sleep. Nothing to do if awake.
     *
     * @retval CommandResponse The response to wait for
     */
    CommandResponse wake_with_response();

    /**
     * @brief Sets flags for the locator module.
     *
     * @param locator_flags The flags to set
     */
    void set_locator_flags(bool locator_flags);

    /**
     * @brief Fills a given region of Sphero BOLT's 8x8 matrix a specific color
     *
     * @param x1 The x coordinate of the first corner
     * @param y1 The y coordinate of the first corner
     * @param x2 The x coordinate of the second corner
     * @param y2 The y coordinate of the second corner
     * @param color The color to set matrix to
     */
    void set_matrix_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, RGBColor color);

    /**
     * @brief Sets Sphero BOLT's LED matrix to specified color
     *
     * @param color The color to set matrix to
     */
    void set_matrix_color(RGBColor color);

    /**
     * @brief Set indivudal pixel on Sphero BOLT's LED matrix to specified color
     * @param[in] x The x coordinate of the pixel
     * @param[in] y The y coordinate of the pixel
     * @param[in] color The color to set the pixel to
     */
    void set_matrix_pixel_color(uint8_t x, uint8_t y, RGBColor color);

    /**
     * @brief Display character on Sphero BOLT's LED matrix
     *
     * @param[in] char The character to display
     * @param[in] color The color to set the pixel to
     */
    void set_matrix_character(unsigned char str, RGBColor color);

    /**
     * @brief Registers a matrix animation
     *
     * @param[in] Frames is a list of frame. Each frame is a list of 8 row, each row is a list of 8 ints (from 0 to 15, index in color palette)
     * @param[in] palette is a list of colors
     * @param[in] fps
     * @param[in] transition to true if fade between frames
     */
    void register_matrix_animation(std::vector<std::vector<std::vector<uint8_t>>> frames, std::vector<RGBColor> palette, uint8_t fps, bool transition);

    /**
     * @brief Saves a compressed frame with a specified index
     *
     * @param[in] index The index of the frame
     * @param[in] frame The frame to save
     */
    void save_compressed_frame(uint8_t index, std::vector<uint8_t> frame);

    /**
     * @brief Saves a compressed frame with a specified index
     *
     * @param[in] index The index of the frame
     * @param[in] frame The frame to save
     *
     * @retval CommandResponse The response to wait for
     */
    CommandResponse save_compressed_frame_with_response(uint8_t index, std::vector<uint8_t> frame);

    /**
     * @brief Save an animation
     *
     * @param[in] fps The frame rate of the animation
     * @param[in] fade_animation Whether or not to fade between frames
     * @param[in] palette The palette of colors to use
     * @param[in] frame_indexes The indexes of frames in the animation
     */
    void save_compressed_frame_animation(uint8_t fps, bool fade_animation, std::vector<RGBColor> palette, std::vector<uint16_t> frame_indexes);

    /**
     * @brief Play an animation
     *
     * @param[in] animation_id The id of the animation
     * @param[in] loop Whether or not to loop the animation
     */
    void play_animation(uint8_t animation_id, bool loop = true);

    /**
     * @brief Sets all the LEDs on Sphero BOLT with a 8 bit mask
     *
     * @param mask The 8 bit mask to set the LEDs with
     */
    void set_all_leds_with_8_bit_mask(uint8_t mask, std::vector<uint8_t> led_values);

    /**
     * @brief Sets LEDs from a map
     *
     * @param mapping The mapping of LEDs to values
     */
    void set_all_leds_with_map(std::unordered_map<LEDs, uint8_t> mapping);

    /**
     * @brief Turns off all LEDs on Sphero BOLT
     *
     * @private Currently we use std::vector but it might be better to use std::array and make the array at compile time
     */
    void turn_off_all_leds();

    /**
     * @brief Drive the sphero
     *
     * @param[in] speed The speed to drive at
     * @param[in] heading The heading to drive at
     */
    void drive(uint8_t speed, uint16_t heading);

    /**
     * @brief Drive the sphero
     *
     * @param[in] speed The speed to drive at
     * @param[in] heading The heading to drive at
     *
     * @retval CommandResponse The response to wait for
     */
    CommandResponse drive_with_response(uint8_t speed, uint16_t heading);

    /**
     * @brief Sets the direction the robot will drive in
     *
     * @note Assuming you aim the robot with the blue tail light facing you, then 0° is forward, 90° is right,
     * 270° is left, and 180° is backward.
     *
     * @param[in] heading The heading to drive at
     */
    void set_heading(uint16_t heading);

    /**
     * @brief Reset aim
     *
     * @note Resets the heading calibration (aim) angle to use the current direction of the robot as 0°
     */
    void reset_aim();

    /**
     * @brief Wait for a packet to be resolved
     *
     * @param response The response to wait for
     *
     * @retval std::optional<Packet> The packet if it was received
     */
    std::optional<Packet> wait_for_response(const CommandResponse& response);
};

#endif // SPHERO_H
