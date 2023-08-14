#ifndef SPHERO_H
#define SPHERO_H

#include "ble/sphero_client.h"
#include "controls/packet.hpp"
#include "controls/packet_collector.hpp"
#include "controls/packet_manager.hpp"
#include "utils/color.hpp"

#define PACKET_PROCESSING_QUEUE_PRIORITY 4

class Sphero {
private:
    /** @brief The id of the Sphero Context */
    uint8_t sphero_id;

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

public:
    PacketManager* packet_manager;

    Sphero(uint8_t id);
    // Sphero(bt_sphero_client* client);
    ~Sphero();

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
};

#endif // SPHERO_H
