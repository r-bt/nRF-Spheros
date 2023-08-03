#ifndef BT_SPHERO_CLIENT_H
#define BT_SPHERO_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

/**
 * Definitions of Sphero BOLT UUIDs
 */

/*Note that UUIDs have Least Significant Byte ordering */
#define SPHERO_SERVICE_UUID 0x21, 0x21, 0x6F, 0x72, 0x65, 0x68, 0x70, 0x53, 0x20, 0x4F, 0x4F, 0x57, 0x01, 0x00, 0x01, 0x00
#define BT_SPHERO_SERVICE_UUID BT_UUID_DECLARE_128(SPHERO_SERVICE_UUID)

#define SPHERO_PACKETS_UUID 0x21, 0x21, 0x6F, 0x72, 0x65, 0x68, 0x70, 0x53, 0x20, 0x4F, 0x4F, 0x57, 0x02, 0x00, 0x01, 0x00
#define BT_SPHERO_PACKETS_UUID BT_UUID_DECLARE_128(SPHERO_PACKETS_UUID)

/**
 * @brief Handles on the connected sphero that are needed to interact
 * */
struct bt_sphero_client_handles {
    /** Handle of the sphero characteristic which packets should be written to*/
    uint16_t packets;
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
int bt_sphero_client_init(struct bt_sphero_client* sphero, const struct bt_sphero_client_init_param* init_param);

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

/** @brief Assign handles to Sphero Client instance
 *
 * Should be called when a connection with a Sphero is established
 *
 * GATT attribute handles are provided by the GATT DB discovery module
 *
 * @param[in] dm Discovery object
 * @param[in, out] sphero Sphero Client instance
 *
 * @retval 0 If successful
 * @retval (-ENOTSUP) Special error code used when UUID mismatch is detected
 * @retval Otherwise, a negative error code is returned
 */
int bt_sphero_handles_assign(struct bt_gatt_dm* dm, struct bt_sphero_client* sphero);

/** @brief Request Sphero to start sending notifcations with packets
 *
 * Enables notifications for the Sphero Packet Characteristic at the peer
 * by writing to the CCC descriptor of the Packet Characteristic
 *
 * @param[in, out] sphero Sphero Client instance
 *
 * @retval 0 If successful
 *         Otherwise, a negative error code is returned
 */
int bt_sphero_subscribe(struct bt_sphero_client* sphero);

#ifdef __cplusplus
}
#endif

#endif /* BT_SPHERO_CLIENT_H */