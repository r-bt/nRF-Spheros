// Kernal
#include <zephyr/kernel.h>
// Bluetooth
#include <bluetooth/conn_ctx.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
// Logging
#include <zephyr/debug/thread_analyzer.h>
#include <zephyr/logging/log.h>
// Clients
#include "ble/sphero_client.h"

/*Note that UUIDs have Least Significant Byte ordering */
#define SPHERO_SERVICE_UUID 0x21, 0x21, 0x6F, 0x72, 0x65, 0x68, 0x70, 0x53, 0x20, 0x4F, 0x4F, 0x57, 0x01, 0x00, 0x01, 0x00
#define BT_SPHERO_SERVICE_UUID BT_UUID_DECLARE_128(SPHERO_SERVICE_UUID)

#define SPHERO_CHARACTERISTIC_UUID 0x21, 0x21, 0x6F, 0x72, 0x65, 0x68, 0x70, 0x53, 0x20, 0x4F, 0x4F, 0x57, 0x02, 0x00, 0x01, 0x00
#define BT_SPHERO_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(SPHERO_CHARACTERISTIC_UUID)

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static struct bt_conn* default_conn;

BT_CONN_CTX_DEF(conns, CONFIG_BT_MAX_CONN, sizeof(struct bt_sphero_client));

static void exchange_func(struct bt_conn* conn, uint8_t err, struct bt_gatt_exchange_params* params)
{
    if (!err) {
        LOG_INF("exchange func: MTU exchange done");
    } else {
        LOG_WRN("exchange func: MTU exchange failed (err %d)", err);
    }
}

static void on_sent(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_write_params* params)
{
    LOG_DBG("Sent had error code: %d", err);
}

static void discovery_complete(struct bt_gatt_dm* dm, void* context)
{
    LOG_INF("Discovery complete");

    // Send the wake up command

    struct bt_conn* conn = bt_gatt_dm_conn_get(dm);

    if (!conn) {
        LOG_ERR("Could not get connection");
        return;
    }

    const struct bt_gatt_dm_attr* gatt_chrc;
    gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_SPHERO_CHARACTERISTIC_UUID);

    if (!gatt_chrc) {
        LOG_ERR("Could not find characteristic");
        return;
    }

    const struct bt_gatt_dm_attr* gatt_desc;
    gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_SPHERO_CHARACTERISTIC_UUID);

    if (!gatt_desc) {
        LOG_ERR("Could not find descriptor");
        return;
    }

    struct bt_gatt_chrc* gatt_chrc_val = bt_gatt_dm_attr_chrc_val(gatt_chrc);

    if (!gatt_chrc_val) {
        LOG_ERR("Could not get characteristic value");
        return;
    }

    LOG_INF("Got characteristic");

    struct bt_gatt_write_params write_params;

    LOG_DBG("Value handle: %d", gatt_chrc_val->value_handle);
    LOG_DBG("DESC handle: %d", gatt_desc->handle);

    unsigned char data[] = { 0x8d, 0xa, 0x13, 0xd, 0x0, 0xd5, 0xd8 };

    write_params.func = on_sent;
    write_params.handle = gatt_desc->handle;
    write_params.offset = 0;
    write_params.data = data;
    write_params.length = 7;

    LOG_INF("Sending command");

    int err;

    err = bt_gatt_write(conn, &write_params);
    if (err) {
        LOG_ERR("Error when sending command");
    }

    bt_gatt_dm_data_release(dm);

    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
    } else {
        LOG_INF("Scanning started");
    }
}

static void discovery_service_not_found(struct bt_conn* conn,
    void* context)
{
    LOG_INF("Service not found");

    int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
    } else {
        LOG_INF("Scanning started");
    }
}

static void discovery_error(struct bt_conn* conn,
    int err,
    void* context)
{
    LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
    .completed = discovery_complete,
    .service_not_found = discovery_service_not_found,
    .error_found = discovery_error,
};

static void gatt_discover(struct bt_conn* conn)
{
    int err;

    err = bt_gatt_dm_start(conn, BT_SPHERO_SERVICE_UUID, &discovery_cb, NULL);

    if (err) {
        LOG_ERR("Could not start the discovery procedure, error: %d", err);
    }
}

static void connected(struct bt_conn* conn, uint8_t conn_err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        LOG_INF("Failed to connect to %s (%d)", addr, conn_err);

        if (default_conn == conn) {
            bt_conn_unref(default_conn);
            default_conn = NULL;

            err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
            if (err) {
                LOG_ERR("Scanning failed to start (err %d)", err);
            }
        }

        return;
    }

    LOG_INF("Connected: %s", addr);

    err = bt_conn_set_security(conn, BT_SECURITY_L1);

    if (err) {
        LOG_ERR("Failed to set security (err %d)", err);

        gatt_discover(conn);
    } else {
        LOG_INF("Security level set");
        gatt_discover(conn);
        LOG_DBG("Discovering GATT database");
    }

    // Stop scanning during discovery

    err = bt_scan_stop();
    LOG_DBG("Stopped scanning");
    if ((!err) && (err != -EALREADY)) {
        LOG_ERR("Stop LE scan failed (err %d)", err);
    }

    // err = bt_scan_stop();
    // if ((!err) && (err != -EALREADY)) {
    //     LOG_ERR("Stop LE scan failed (err %d)", err);
    // }
}

static void disconnected(struct bt_conn* conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected: %s (reason %u)", addr,
        reason);

    err = bt_conn_ctx_free(&conns_ctx_lib, conn);

    if (err) {
        LOG_WRN("The connection context could not be freed (err %d)", err);
    }

    bt_conn_unref(conn);
    default_conn = NULL;
}

static void security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        LOG_INF("Security changed: %s level %u", addr, level);
    } else {
        LOG_WRN("Security failed: %s level %u err %d", addr,
            level, err);
    }

    gatt_discover(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

static void scan_filter_match(struct bt_scan_device_info* device_info, struct bt_scan_filter_match* filter_match, bool connectable)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

    LOG_INF("Filters matched. Address: %s connectable: %d",
        addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info* device_info)
{
    LOG_ERR("Connecting failed");
}

#define NAME_LEN 30

static bool data_cb(struct bt_data* data, void* user_data)
{
    uint8_t len;
    char* name = user_data;

    switch (data->type) {
    case BT_DATA_NAME_SHORTENED:
    case BT_DATA_NAME_COMPLETE:
        len = MIN(data->data_len, NAME_LEN - 1);
        memcpy(name, data->data, len);
        name[len] = '\0';
        return false;
    default:
        return true;
    }
}

static void scan_connecting(struct bt_scan_device_info* device_info, struct bt_conn* conn)
{
    char name[NAME_LEN];

    (void)memset(name, 0, sizeof(name));

    bt_data_parse(device_info->adv_data, data_cb, name);

    LOG_DBG("Connecting to %s", name);

    default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, scan_connecting);

static int scan_init(void)
{
    int err;
    struct bt_scan_init_param scan_init = {
        .connect_if_match = 1,
        .scan_param = BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE,
            BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW),
    };

    bt_scan_init(&scan_init);
    bt_scan_cb_register(&scan_cb);

    int sphero_names_len = 15;

    char* sphero_names[] = {
        "SB-1B35",
        "SB-F860",
        "SB-2175",
        "SB-3026",
        "SB-618E",
        "SB-6B58",
        "SB-9938",
        "SB-BFD4",
        "SB-C1D2",
        "SB-CEFA",
        "SB-DF1D",
        "SB-F465",
        "SB-F479",
        "SB-F885",
        "SB-FCB2"
    };

    for (int i = 0; i < sphero_names_len; i++) {
        err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, sphero_names[i]);
        if (err) {
            LOG_ERR("Scanning filters cannot be set (err %d)", err);
            return err;
        }
    }

    err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
    if (err) {
        LOG_ERR("Filters cannot be turned on (err %d)", err);
        return err;
    }

    LOG_INF("Scan module initialized");
    return err;
}

static void auth_cancel(struct bt_conn* conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn* conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing complete: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn* conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing failed conn: %s, reason %d", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed,
};

int main()
{
    int err;

    err = bt_conn_auth_cb_register(&conn_auth_callbacks);
    if (err) {
        LOG_ERR("Failed to register authorization callback (err %d)", err);
        return 0;
    }

    err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
    if (err) {
        LOG_ERR("Failed to register authorization info callback (err %d)", err);
        return 0;
    }

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return 0;
    }

    LOG_INF("Bluetooth initialized");

    err = scan_init();
    if (err != 0) {
        LOG_ERR("Scan module failed to initialize (err %d)", err);
        return 0;
    }

    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return 0;
    }

    LOG_INF("Scanning successfully started");

    return 1;
}