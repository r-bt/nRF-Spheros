#include "ble_peripheral.h"
#include "swarmalator_service.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(BLE_PERIPHERAL, LOG_LEVEL_INF);

// BLE Peripheral

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_128_ENCODE(0xbf5418ec, 0x10b1, 0x4b35, 0x1523, 0xb4e4f241f4fa)),
};

static struct bt_le_adv_param* adv_param = BT_LE_ADV_PARAM(
    BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY,
    800,
    801,
    NULL);

void on_connected(struct bt_conn* conn, uint8_t err)
{
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);

    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        return;
    }

    LOG_INF("Connected as Peripheral");
}

void on_disconnected(struct bt_conn* conn, uint8_t err)
{
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);

    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        return;
    }

    LOG_INF("Disconnected as Peripheral");
}

struct bt_conn_cb connection_callbacks = {
    .connected = on_connected,
    .disconnected = on_disconnected,
};

// Swarmalator Packet Characteristic Callback
static void swarmalator_packet_cb()
{
    LOG_INF("Swarmalator Packet Callback");
}

int bt_peripheral_init(struct swarmalator_cb_struct* swarmalator_cb)
{
    int err;

    bt_addr_le_t addr;
    err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);

    if (err) {
        LOG_ERR("Failed to set address, err %d", err);
        return err;
    }

    err = bt_id_create(&addr, NULL);
    if (err < 0) {
        LOG_ERR("Bluetooth ID creation failed (err %d)", err);
        return err;
    }

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)\n", err);
        return err;
    }

    bt_conn_cb_register(&connection_callbacks);

    // Register application callbacks with Swarmalator service
    err = bt_swarmalator_service_init(swarmalator_cb);
    if (err) {
        LOG_ERR("Failed to init Swarmalator Servicde (err: %d)\n", err);
        return;
    }
    LOG_INF("Swarmalator Service initialized");

    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("Advertising successfully started");

    return err;
}