/*
 * Description:
 *
 * Source file to initiate and establish connection with a device that complies
 * to some specifications
 *
 */

// --- includes ----------------------------------------------------------------
#include "ble_conn_control.h"
#include "ble_characteristic_control.h"
#include "ble_fsm.h"
#include "../../common/common.h"
#include "../measurements/measurements_fsm.h"

#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <logging/log.h>

#include <sys/printk.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>

// --- defines -----------------------------------------------------------------
// TODO: 600 might be causing an issue where the central node is informed about
// a disconnection late
#define BT_CONNECTION_PARAMETERS BT_LE_CONN_PARAM(0x40, 0x55, 4, 600)

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(ble_m);

// --- static variables definitions --------------------------------------------
// BLE connection data (contains the ble handle)
static ble_connection_data_t *ble_connection_data;

// --- static functions declarations -------------------------------------------
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad);
static bool connectable_device_found(struct bt_data *data, void *user_data);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void connected(struct bt_conn *conn, uint8_t conn_err);
static void disconnected(struct bt_conn *conn, uint8_t reason);

// --- static functions definitions --------------------------------------------
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/**
 * @brief Callback function that is called when a ble device is found after start scan
 *        If the device is connectable, the connectable_device_found() cb function is called
 *        to filter the devices that support the desired services
 * 
 * @param addr 
 * @param rssi 
 * @param type 
 * @param ad 
 */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad)
{
    // Filter devices that we can connect to
    if (type == BT_GAP_ADV_TYPE_ADV_IND ||
        type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND)
    {
        // After finding a connectable device, filter its UUID with a cb function
        bt_data_parse(ad, connectable_device_found, (void *)addr);
    }
}

/**
 * @brief After finding a connectable device, check if it supports the desired service
 *        in order to connect to it (or skip)
 * 
 * @param data 
 * @param user_data 
 * @return true 
 * @return false 
 */
static bool connectable_device_found(struct bt_data *data, void *user_data)
{
    bt_addr_le_t *addr = user_data;
    int i;

    // This switch checks the type_of_data on the ad[] map (check BT_DATA_BYTES)
    // depending on the data type, handle the data in a different way
    // if we add BT_DATA_BYTES(BT_DATA_UUID16_ALL, ...) we need extra
    // case BT_DATA_UUID16_ALL to parse the 16bit uuid in a different way
    switch (data->type)
    {
        // --------- Measurement service discovery on connectable device
        // Filter out devices with 128bit UUIDs
    case BT_DATA_UUID128_SOME:
    case BT_DATA_UUID128_ALL:
        // Check the advertised data length
        if (data->data_len % BT_UUID_SIZE_128 != 0U)
        {
            // TODO: debug instead of print
            LOG_INF("AD malformed");
            return true;
        }

        for (i = 0; i < data->data_len; i += BT_UUID_SIZE_128)
        {
            uint8_t received_uuid[BT_UUID_SIZE_128];
            // TODO: MEASUREMENT_SERVICE_UUID can be outside of the function
            // probably pass it as an argument to start scan to make module
            // more generic
            uint8_t service_uuid[BT_UUID_SIZE_128] = {MEASUREMENT_SERVICE_UUID};
            int err;

            // Storing the received UUID on a buffer
            memcpy(received_uuid, &data->data[i], BT_UUID_SIZE_128);
            // Compare received UUID with desired service UUID
            if (memcmp(received_uuid, service_uuid, BT_UUID_SIZE_128))
            {
                // Continue to the next service UUID of the found device
                continue;
            }

            // TODO: why stop the scan?
            err = bt_le_scan_stop();
            if (err)
            {
                LOG_INF("Stop LE scan failed (err %d)", err);
                continue;
            }

            // Create the connection to the device that supports the desired service
            // Note: after creating connection successfully, connected callback
            // will be called
            err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                    BT_CONNECTION_PARAMETERS, &ble_connection_data->ble_connection_handle);
            if (err)
            {
                LOG_INF("Create conn failed (err %d)", err);
                k_sem_give(&ble_connect_ok_sem);
            }

            return false;
        }
        // --------- Measurement service discovery on connectable device
    }

    return true;
}

/**
 * @brief Callback function that is called when a connection is established
 * 
 * @param conn connection handle
 * @param conn_err 
 */
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    char *log_addr;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err)
    {
        log_addr = log_strdup(addr);
        LOG_INF("Failed to connect to %s (%u)", log_addr, conn_err);

        bt_conn_unref(conn);
        conn = NULL;

        k_sem_give(&ble_connect_ok_sem);
        return;
    }
    // Set connected flag to true after connection is established
    ble_connection_data->is_connected = true;
    // Store mac address of device
    memcpy(ble_connection_data->mac_address, addr, sizeof(char) * MAC_ADDRESS_LENGTH);

    k_sem_give(&ble_connect_ok_sem);
    k_sem_give(&at_least_one_active_connection_sem);
}

/**
 * @brief Callback function that is called when a connection is disrupted
 *        remove_connection_data() to remove the connection data of the
 *        disconnected device
 * 
 * @param conn Connection handle
 * @param reason 
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    // --- logging ---
    char addr[BT_ADDR_LE_STR_LEN];
    char *log_addr;
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    log_addr = log_strdup(addr);
    LOG_INF("Disconnected: %s, reason: %d", log_addr, reason);

    // --- remove connection ---
    remove_connection_data(conn);
    k_sem_give(&ble_wait_for_disconnect_sem);
    k_sem_take(&at_least_one_active_connection_sem, K_NO_WAIT);
}

// --- function definitions ----------------------------------------------------
/**
 * @brief Function to start searching for ble devices to connect
 *        Fetches an available device from static ble_connection_data_t bluetooth_devices
 * 
 * @param scan_parameters Scan parameters
 * @return ble_connection_data_t* 
 */
ble_connection_data_t *start_scan(const struct bt_le_scan_param *scan_parameters)
{
    int err;

    // Fetch an available device from static ble_connection_data_t bluetooth_devices
    ble_connection_data = get_empty_ble_handle();

    // If this condition is true, device array is full
    if (ble_connection_data == NULL)
    {
        // Handle NULL return by waiting for a device to disconnect and then start scan again
        return NULL;
    }

    // Start searching for connectable devices. If a connectable device is found,
    // device_found() cb will be called to check if this device supports the services
    // needed by the central_node
    // TODO: is bt_le_scan_start until connection? If not, returning ble_connection_data could be problematic
    err = bt_le_scan_start(scan_parameters, device_found);
    if (err)
    {
        LOG_INF("Scanning failed to start (err %d)", err);
        return NULL;
    }
    LOG_INF("Scan started");
    return ble_connection_data;
}

/**
 * @brief CB function that is called just to notify (via a semaphore) that bluetooth
 *        started succesfully
 * 
 * @param err 
 */
void bt_ready(int err)
{
    k_sem_give(&ble_init_ok_sem);
    if (err)
    {
        LOG_INF("Error while enabling bluetooth (err %d)", err);
    }
}