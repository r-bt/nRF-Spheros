#include "scanner.h"
// Bluetooth
#include <bluetooth/conn_ctx.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
// Logging
#include <zephyr/logging/log.h>
// Clients
#include "sphero_client.h"

LOG_MODULE_REGISTER(scanner, LOG_LEVEL_DBG);

/**
 * GLOBAL variables
 */

BT_CONN_CTX_DEF(conns, CONFIG_BT_MAX_CONN, sizeof(struct bt_sphero_client));

#define NAME_LEN 30

static struct bt_conn* default_conn;

uint64_t last_sphero_found;

/**
 * Service discovery
 */

static void discovery_complete(struct bt_gatt_dm* dm, void* context)
{
    LOG_INF("Discovery complete");

    struct bt_sphero_client* sphero = context;

    bt_sphero_handles_assign(dm, sphero);

    bt_gatt_dm_data_release(dm);

    int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);

    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
    } else {
        LOG_INF("Scanning started");
        last_sphero_found = k_uptime_get();
    }

    // // Send wake up message

    // unsigned char data[] = { 0x8d, 0xa, 0x13, 0xd, 0x0, 0xd5, 0xd8 };

    // LOG_INF("Sending command");

    // bt_sphero_client_send(sphero, data, sizeof(data));
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

    struct bt_sphero_client* sphero_client = bt_conn_ctx_get(&conns_ctx_lib, conn);

    if (!sphero_client) {
        return;
    }

    err = bt_gatt_dm_start(conn, BT_SPHERO_SERVICE_UUID, &discovery_cb, sphero_client);

    if (err) {
        LOG_ERR("Could not start the discovery procedure, error: %d", err);
    }

    bt_conn_ctx_release(&conns_ctx_lib, (void*)sphero_client);
}

/*
 * Connected code
 */

static void connected(struct bt_conn* conn, uint8_t conn_err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    struct bt_sphero_client_init_param init = {};

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

    // Allocate memory for this connection using the connection context library

    struct bt_sphero_client* sphero_client = bt_conn_ctx_alloc(&conns_ctx_lib, conn);

    if (!sphero_client) {
        LOG_WRN("There is no free memory to allocate the connection context");
    }

    memset(sphero_client, 0, bt_conn_ctx_block_size_get(&conns_ctx_lib));

    err = bt_sphero_client_init(sphero_client, &init);

    bt_conn_ctx_release(&conns_ctx_lib, (void*)sphero_client);

    if (err) {
        LOG_ERR("Sphero client initalization failed (err %d)", err);
    } else {
        LOG_INF("Sphero Clinet module initalized");
    }

    gatt_discover(conn);

    // Stop scanning during discovery

    err = bt_scan_stop();
    if ((!err) && (err != -EALREADY)) {
        LOG_ERR("Stop LE scan failed (err %d)", err);
    }
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

/*
 * SCANNING CODE
 */

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

static void scan_connecting_error(struct bt_scan_device_info* device_info)
{
    LOG_ERR("Connecting failed");
}

static void scan_filter_match(struct bt_scan_device_info* device_info, struct bt_scan_filter_match* filter_match, bool connectable)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

    LOG_INF("Filters matched. Address: %s connectable: %d",
        addr, connectable);
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

static int scanner_scan_init(void)
{
    int err;
    struct bt_scan_init_param scan_init = {
        .connect_if_match = 1,
        .scan_param = BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE,
            BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW),
    };

    bt_scan_init(&scan_init);
    bt_scan_cb_register(&scan_cb);

    int sphero_names_len = 5;

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

int scanner_init(void)
{
    int err;

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    err = scanner_scan_init();

    if (err != 0) {
        LOG_ERR("Scan module failed to initialize (err %d)", err);
        return err;
    }

    return 0;
}

int scanner_start()
{
    last_sphero_found = k_uptime_get();
    int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return err;
    }

    return 0;
}

int scanner_stop()
{
    int err = bt_scan_stop();
    if (err) {
        LOG_ERR("Stop LE scan failed (err %d)", err);
        return err;
    }

    return 0;
}

unsigned int scanner_get_sphero_count()
{
    return bt_conn_ctx_count(&conns_ctx_lib);
}

struct bt_sphero_client* scanner_get_sphero(uint8_t id)
{
    const struct bt_conn_ctx* ctx = bt_conn_ctx_get_by_id(&conns_ctx_lib, id);

    struct bt_sphero_client* sphero = ctx->data;

    return sphero;
}

void scanner_release_sphero(struct bt_sphero_client* sphero)
{
    bt_conn_ctx_release(&conns_ctx_lib, sphero);
}