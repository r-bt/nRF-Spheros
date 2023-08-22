#ifndef BLE_PERIPHERAL_H
#define BLE_PERIPHERAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "swarmalator_service.h"

/**
 * @brief Initialize the BLE peripheral
 *
 */
int bt_peripheral_init(struct swarmalator_cb_struct* swarmalator_cb);

#ifdef __cplusplus
}
#endif
#endif