// --- includes ----------------------------------------------------------------
#include "ble_connection_data.h"
#include "ble_characteristic_control.h"

#include <stdint.h>
#include <stdio.h>

#include <logging/log.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(ble_m);

// --- static variables definitions --------------------------------------------
// All connection data are stored on this static variable
static ble_connection_data_t bluetooth_devices[BLE_MAX_CONNECTIONS];

// --- function definitions ----------------------------------------------------
/**
 * @brief Get a free bluetooth_device space to store new connection data
 *        This function is called every time central node starts scanning
 *        for devices to connect to. If a device is found, the connection data
 *        of this device are stored to the bluetooth_device member returned by 
 *        this function
 * 
 * @return free bluetooth_device member
 */
ble_connection_data_t *get_empty_ble_handle(void)
{
    int ble_device_index = 0;
    // Find a free space on bluetooth_device array
    for (ble_device_index = 0; ble_device_index < BLE_MAX_CONNECTIONS; ble_device_index++)
    {
        if (!bluetooth_devices[ble_device_index].is_connected)
        {
            break;
        }
    }

    // get_empty_ble_handle_index returns index = BLE_MAX_CONNECTIONS when
    // no empty connection slots are found
    if (ble_device_index == BLE_MAX_CONNECTIONS)
    {
        // If return is null, it means we reached the maximum connection number
        return NULL;
    }

    return &bluetooth_devices[ble_device_index];
}

/**
 * @brief Clear connection data of a connection data item after disconnection
 * 
 * @param conn 
 */
void remove_connection_data(struct bt_conn *conn)
{
    ble_connection_data_t *conn_data;
    conn_data = get_device_by_conn_handle(conn);

    if (conn_data == NULL)
    {
        LOG_INF("Invalid connection handle");
        return;
    }

    // --- clear the connection data
    // Remove connection
    bt_conn_unref(conn);
    memset(conn_data, 0, sizeof(ble_connection_data_t));
}

/**
 * @brief Get the device (from static ble_connection_data_t bluetooth_devices) 
 *        that corresponds to the given connection handle
 * 
 * @param conn Connection handle
 * @return bluetooth device member
 */
ble_connection_data_t *get_device_by_conn_handle(struct bt_conn *conn)
{
    int index = 0;
    bool device_found = false;
    for (index = 0; index < BLE_MAX_CONNECTIONS; index++)
    {
        if (bluetooth_devices[index].ble_connection_handle == conn)
        {
            device_found = true;
            break;
        }
    }
    if (device_found)
    {
        return &bluetooth_devices[index];
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Get ble connection handle existing on bluetooth_devices[] by index
 * 
 * @param index 
 * @return struct bt_conn* 
 */
struct bt_conn *get_ble_conn_handles(uint8_t index)
{
    if(index < BLE_MAX_CONNECTIONS)
    {
        return bluetooth_devices[index].ble_connection_handle;
    }
    else
    {
        LOG_INF("Wrong ble device index %d", index);
        return NULL;
    }
}

/**
 * @brief Function to retreive the mac address of a bluetooth connection
 * 
 * @param conn 
 * @return the mac address corresponding to the connection handle
 * @return NULL if the connection handle is not found on the array of ble devices
 */
char *get_mac_address_by_conn_handle(struct bt_conn* conn)
{
    for(int index = 0; index < BLE_MAX_CONNECTIONS; index++)
    {
        if(bluetooth_devices->ble_connection_handle == conn)
        {
            return bluetooth_devices->mac_address;
        }
    }

    return NULL;
}