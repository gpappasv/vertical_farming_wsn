/*
 * Description:
 *
 * Source file that initiates and controls the process of bluetooth connection
 * and characteristic discovery
 *
 */

// --- includes ----------------------------------------------------------------
#include "ble_fsm.h"
#include "ble_conn_control.h"
#include "ble_characteristic_control.h"

#include <zephyr.h>
#include <logging/log.h>
#include <sys/printk.h>
#include <bluetooth/bluetooth.h>
#include <smf.h>
#include <stdbool.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(ble_m);

// --- enums -------------------------------------------------------------------
// List of states
enum ble_state_e
{
    // First initialize bluetooth
    BLE_INIT_STATE,
    // Find a device to connect to
    BLE_CONNECT_STATE,
    // Discover the desired characteristics of the device we just connected to
    BLE_CHAR_DISCOVER_STATE,
    // Wait here if max ble connections reached
    BLE_WAIT_FOR_DISCONNECT
};

// --- static variables definitions --------------------------------------------
// Scan parameters
static struct bt_le_scan_param scan_param = {
    .type = BT_LE_SCAN_TYPE_ACTIVE,
    .options = BT_LE_SCAN_OPT_NONE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};

// Forward declaration of state table
static const struct smf_state ble_states[];

// --- variables definitions ---------------------------------------------------
struct k_sem ble_init_ok_sem;
struct k_sem ble_connect_ok_sem;
struct k_sem ble_char_discovery_sem;
struct k_sem ble_wait_for_disconnect_sem;

// User defined object
struct user_object_s
{
    // This must be first
    struct smf_ctx ctx;

    // Other state specific data add here
    ble_connection_data_t *active_connection_data;
} user_object;

// --- static function declarations --------------------------------------------
static void ble_init_state_entry(void *o);
static void ble_init_state_run(void *o);
static void ble_init_state_exit(void *o);

static void ble_connect_state_run(void *o);

static void ble_char_discover_state_run(void *o);

static void ble_wait_for_disconnect_entry(void *o);
static void ble_wait_for_disconnect_run(void *o);

// --- static function definitions ---------------------------------------------
// --- State BLE INIT
static void ble_init_state_entry(void *o)
{
    k_sem_init(&ble_init_ok_sem, 0, 1);
    k_sem_init(&ble_connect_ok_sem, 0, 1);
    k_sem_init(&ble_char_discovery_sem, 0, 1);
    k_sem_init(&ble_wait_for_disconnect_sem, 0, 1);

    bt_enable(bt_ready);
}
static void ble_init_state_run(void *o)
{
    int err;
    err = k_sem_take(&ble_init_ok_sem, K_MSEC(500));
    if (err == -EAGAIN)
    {
        // TODO: add system reset
        LOG_INF("Ble could not start (err %d)", err);
    }
    smf_set_state(SMF_CTX(&user_object), &ble_states[BLE_CONNECT_STATE]);
}
static void ble_init_state_exit(void *o)
{
    LOG_INF("Ble initialized");
}

// --- State BLE CONNECT
static void ble_connect_state_run(void *o)
{
    struct user_object_s *user_ctx = (struct user_object_s *)o;
    // Work
    user_ctx->active_connection_data = start_scan(&scan_param);
    // bluetooth_devices array is full, wait for someone to disconnect
    if (user_ctx->active_connection_data == NULL)
    {
        smf_set_state(SMF_CTX(&user_object), &ble_states[BLE_WAIT_FOR_DISCONNECT]);
    }
    else
    {
        // Wait until connection happens
        k_sem_take(&ble_connect_ok_sem, K_FOREVER);
        // Set next state
        smf_set_state(SMF_CTX(&user_object), &ble_states[BLE_CHAR_DISCOVER_STATE]);
    }
}

// --- State BLE CHAR DISCOVER
static void ble_char_discover_state_run(void *o)
{
    struct user_object_s *user_ctx = (struct user_object_s *)o;

    // is_connected will only become true if connection is established without errors
    // if connection is established without errors, proceed with characteristic discovery
    if (user_ctx->active_connection_data->is_connected)
    {
        // Discover every characteristic of the measurement service
        for(int measurement_service_characteristic_count = 0; measurement_service_characteristic_count < MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT; measurement_service_characteristic_count++)
        {
            characteristic_discovery_wrapper(user_ctx->active_connection_data->ble_connection_handle, MEASUREMENT_SERVICE_INDEX, measurement_service_characteristic_count);
            k_sem_take(&ble_char_discovery_sem, K_MSEC(1000));
        }

        // Discover every characteristic of the configure service
        // Configure service characteristic indexes should be offseted by MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT as they are after measurement service characteristics
        for(int configure_service_char_count = 0 + MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT; configure_service_char_count < CONFIGURE_SERVICE_CHARACTERISTIC_COUNT + MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT; configure_service_char_count++)
        {
            characteristic_discovery_wrapper(user_ctx->active_connection_data->ble_connection_handle, CONFIGURE_SERVICE_INDEX, configure_service_char_count);
            k_sem_take(&ble_char_discovery_sem, K_MSEC(1000));
        }

        smf_set_state(SMF_CTX(&user_object), &ble_states[BLE_CONNECT_STATE]);
    }
    else
    {
        // Right now we get an error while trying to connect if a node resets for some reason (for example press reset button)
        // If something like that happens, then we need to wait for central node to be notified that this node disconnected
        // If we dont wait and keep trying to connect to this node, the connection attempts will always return error
        // TODO: Waiting for the node to disconnect might not be the best approach but for now it works. It depends on the reason
        // the connection failed.
        smf_set_state(SMF_CTX(&user_object), &ble_states[BLE_WAIT_FOR_DISCONNECT]);
    }
}

// --- state BLE WAIT FOR DISCONNECT
// Resets semaphore
static void ble_wait_for_disconnect_entry(void *o)
{
    k_sem_reset(&ble_wait_for_disconnect_sem);
}
static void ble_wait_for_disconnect_run(void *o)
{
    // Wait until someone disconnects
    k_sem_take(&ble_wait_for_disconnect_sem, K_FOREVER);

    smf_set_state(SMF_CTX(&user_object), &ble_states[BLE_CONNECT_STATE]);
}

// Populate state table
static const struct smf_state ble_states[] = {
    [BLE_INIT_STATE] = SMF_CREATE_STATE(ble_init_state_entry, ble_init_state_run, ble_init_state_exit),
    [BLE_CONNECT_STATE] = SMF_CREATE_STATE(NULL, ble_connect_state_run, NULL),
    [BLE_CHAR_DISCOVER_STATE] = SMF_CREATE_STATE(NULL, ble_char_discover_state_run, NULL),
    [BLE_WAIT_FOR_DISCONNECT] = SMF_CREATE_STATE(ble_wait_for_disconnect_entry, ble_wait_for_disconnect_run, NULL),
};

/**
 * @brief Bluetooth FSM -> Takes care of the bluetooth connection and disconnection process
 * 
 */
void ble_fsm(void)
{
    int32_t ret;

    // Set initial state
    smf_set_initial(SMF_CTX(&user_object), &ble_states[BLE_INIT_STATE]);

    // Run the state machine
    while (1)
    {
        // State machine terminates if a non-zero value is returned
        ret = smf_run_state(SMF_CTX(&user_object));
        if (ret)
        {
            // handle return code and terminate state machine
            break;
        }
    }
}

K_THREAD_DEFINE(ble_fsm_id, STACKSIZE, ble_fsm, NULL, NULL, NULL,
                PRIORITY, 0, 0);
