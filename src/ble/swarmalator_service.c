#include <zephyr/types.h>

#include <zephyr/bluetooth/gatt.h>

#include "swarmalator_service.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(swarmalator_service, LOG_LEVEL_DBG);

static struct swarmalator_cb_struct swarmalator_cb;

static ssize_t write_packet(struct bt_conn* conn, const struct bt_gatt_attr* attr, const void* buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, (void*)conn);

    if (offset != 0) {
        LOG_DBG("Invalid offset");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (swarmalator_cb.packet_cb) {
        uint8_t* data = (uint8_t*)buf;

        LOG_DBG("Running callback");
        swarmalator_cb.packet_cb(data, len, swarmalator_cb.packet_cb_context);
    }

    return len;
}

BT_GATT_SERVICE_DEFINE(swarmalator_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_SWARMALATOR),
    // Packet characteristic
    BT_GATT_CHARACTERISTIC(
        BT_UUID_SWARMALATOR_PACKET,
        BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_WRITE,
        NULL, write_packet, NULL),

);

int bt_swarmalator_service_init(struct swarmalator_cb_struct* callbacks)
{
    if (callbacks) {
        swarmalator_cb.packet_cb = callbacks->packet_cb;
        swarmalator_cb.packet_cb_context = callbacks->packet_cb_context;
    }

    return 0;
}