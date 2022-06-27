// --- includes ----------------------------------------------------------------
#include "ble_conn_control.h"
#include "../../common/common.h"

#include <logging/log.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(ble_m);

// --- static functions declarations -------------------------------------------
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void bt_ready(int err);
static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param);
static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout);

// --- static variables definitions --------------------------------------------
static struct bt_conn *ble_connection;

// TODO: Create a separate wrapper to adv operating data after configuration is finished
static const struct bt_data operating_adv_data[] =
{
    // BT_DATA_BYTES(type_of_data -> type of data in the buffer, buffer)
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, MEASUREMENT_SERVICE_UUID),
};

// wrapper to advertise configuration data at first. Should proceed to the next
// fsm state (of operating mode). Maybe turn bluetooth off and back on
// Check: bt_le_ext_adv_set_data()
static const struct bt_data configuration_adv_data[] =
{
    // BT_DATA_BYTES(type_of_data -> type of data in the buffer, buffer)
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, CONFIGURE_SERVICE_UUID),
};

static const struct bt_data sd[] =
{
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static K_SEM_DEFINE(ble_init_ok, 0, 1);

// --- structs -----------------------------------------------------------------
static struct bt_conn_cb conn_callbacks =
{
    .connected = connected,
    .disconnected = disconnected,
    .le_param_req = le_param_req,
    .le_param_updated = le_param_updated
};

// -- static functions definitions ---------------------------------------------
static void connected(struct bt_conn *conn, uint8_t err)
{
    ble_connection = conn;

    if (err)
    {
        LOG_INF("Connection failed (err %u)\n", err);
        return;
    }
    LOG_INF("Connection succeed (err %u)\n", err);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)\n", reason);
}

static void bt_ready(int err)
{
    if (err)
    {
        LOG_INF("BLE init failed with error code %d\n", err);
        return;
    }

    // Configure connection callbacks
    bt_conn_cb_register(&conn_callbacks);

    k_sem_give(&ble_init_ok);
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
    // If acceptable params, return true, otherwise return false.
    return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    struct bt_conn_info info;

    if (bt_conn_get_info(conn, &info))
    {
        LOG_INF("Could not parse connection info\n");
    }
    else
    {
        LOG_INF("Connection parameters updated!");
    }
}

// --- functions definitions ---------------------------------------------------
// TODO: should return error code
void ble_init(void)
{
    int error = 0;

    error = bt_enable(bt_ready);
    if (error)
    {
        LOG_INF("BLE initialization failed\n");
    }

    /* 	Bluetooth stack should be ready in less than 100 msec. 								\
                                                                                            \
        We use this semaphore to wait for bt_enable to call bt_ready before we proceed 		\
        to the main loop. By using the semaphore to block execution we allow the RTOS to 	\
        execute other tasks while we wait. 
    */
    error = k_sem_take(&ble_init_ok, K_MSEC(500));

    if (!error)
    {
        LOG_INF("Bluetooth initialized\n");
    }
    else
    {
        LOG_INF("BLE initialization did not complete in time: (err: %d)\n", error);
    }
}

struct bt_conn* get_ble_connection(void)
{
    return ble_connection;
}

// Function to start advertising configuration data
// This function is called when an unconfigured device boots
void start_configure_state_adv(void)
{
    int err;
    // Start advertising both advertising data and scan response data
    err = bt_le_adv_start(BT_LE_ADV_CONN, configuration_adv_data, ARRAY_SIZE(configuration_adv_data),
                          sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_INF("Advertising failed to start (err %d)\n", err);
        return;
    }

    LOG_INF("Advertising successfully started\n");
}

// Function to start advertising operating data
// This function will be called when device reboots but is already configured
void start_operating_state_adv(void)
{
    int err;
    // Start advertising both advertising data and scan response data
    err = bt_le_adv_start(BT_LE_ADV_CONN, operating_adv_data, ARRAY_SIZE(operating_adv_data),
                          sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_INF("Advertising failed to start (err %d)\n", err);
        return;
    }

    LOG_INF("Advertising successfully started\n");
}
