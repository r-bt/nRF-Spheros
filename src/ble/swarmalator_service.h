#ifndef BT_SWARMALTOR_SERVICE_H
#define BT_SWARMALTOR_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/**
 * Define UUIDs for GATT service and characteristic
 */

/** @brief Swarmalator service UUID */
#define BT_SWARMALATOR_SERVICE_UUID BT_UUID_128_ENCODE(0xbf5418ec, 0x10b1, 0x4b35, 0x1523, 0xb4e4f241f4fa)

/** @brief Packet Characteristic UUID */
#define BT_SWARMALATOR_PACKETS_UUID BT_UUID_128_ENCODE(0xbf5418ed, 0x10b1, 0x4b35, 0x1523, 0xb4e4f241f4fa)

#define BT_UUID_SWARMALATOR BT_UUID_DECLARE_128(BT_SWARMALATOR_SERVICE_UUID)
#define BT_UUID_SWARMALATOR_PACKET BT_UUID_DECLARE_128(BT_SWARMALATOR_PACKETS_UUID)

/** @brief Callback type for when packet is received */
typedef void bt_swarmalator_packet_cb_t(uint8_t* data, uint16_t len, void* context);

/** @brief Callback struct used by the Swarmalator service */
struct swarmalator_cb_struct {
    bt_swarmalator_packet_cb_t* packet_cb;
    void* packet_cb_context;
};

/**
 * @brief Initialize the Swarmalator service
 *
 * This function registers the application callback functions with the Swarmalator service.
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *                      used by the service.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned
 */
int bt_swarmalator_service_init(struct swarmalator_cb_struct* callbacks);

#ifdef __cplusplus
}
#endif

#endif
