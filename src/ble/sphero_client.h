#ifndef BT_SPHERO_CLIENT_H
#define BT_SPHERO_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

/**
 * @brief Handles on the connected sphero that are needed to interact
 * */

struct bt_sphero_client_handles {
    /** Handle of the sphero service which packets should be written to*/
    uint16_t sphero_service
};

struct bt_sphero_client;

/** @brief Sphero Client callback strucutre */

struct bt_sphero_client_cb {
    /** @brief Data received callback
     *
     * Data received as a notification of the sphero packet characteristic
     *
     * @param[in] sphero Sphero Client instance
     * @param[in] data Recieved data
     * @param[in] len Length of the data
     *
     * @retval BT_GATT_ITER_CONTINUE To keep notifications enabled
     * @retval BT_GATT_ITER_STOP To disable notifications
     */
    uint8_t (*received)(struct bt_sphero_client* sphero, const uint8_t* data, uint16_t len);

    /** @brief Data sent callback
     *
     * Data sent as a write to the sphero packet characteristic
     *
     * @param[in] sphero Sphero Client instance
     * @param[in] err ATT error code
     * @param[in] data Sent data
     * @param[in] len Length of the data
     */
    void (*sent)(struct bt_sphero_client* sphero, uint8_t err, const uint8_t* data, uint16_t len);

    /** @brief Sphero Packet notifcations disabled callback
     *
     * @param[in] sphero Sphero Client instance
     */
    void (*unsubscribed)(struct bt_sphero_client* sphero);
};

/**
 * @brief Sphero Client Structure
 */

struct bt_sphero_client {
    struct bt_conn* conn;

    atomic_t state;

    /**
     * Handles on the connected device that are needed to interact with it
     */
    struct bt_sphero_client_handles handles;

    /** GATT subscribe parameters for Sphero Packet Characteristic */
    struct bt_gatt_subscribe_params sphero_packet_subscribe_params;

    /** GATT write parameters for Sphero Packet Characteristics */
    struct bt_gatt_write_params sphero_packet_write_params;

    /** Application callbacks */
    struct bt_sphero_client_cb cb;
};

/** @brief Sphero Client Initaliation Strucutre */
struct bt_sphero_client_init_param {
    struct bt_sphero_client_cb cb;
};

/** @brief Initalize the Sphero Client module
 *
 * This function initializes the Sphero Client module with callbacks defined by
 * the user
 *
 * @param[in, out] sphero Sphero Client instance
 * @param[in] init_param Sphero Client initialization parameters
 *
 * @retval 0 If successful
 *         Otherwise, a negative error code is returned
 */

int bt_sphero_client_init(struct bt_sphero_client* sphero, struct bt_sphero_client_init_param* init_param);

/** @brief Send data to the Sphero
 *
 * This function writes to the Sphero Packet Characteristic
 *
 * @note This procedure is async. The data must remain valid while the function is active
 *
 * @param[in, out] sphero Sphero Client instance
 * @param[in] data Data to be sent
 * @param[in] len Length of the data
 *
 * @retval 0 If successful
 *         Otherwise, a negative error code is returned
 */

int bt_sphero_client_send(struct bt_sphero_client* sphero, const uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* BT_SPHERO_CLIENT_H */