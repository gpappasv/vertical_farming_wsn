#ifndef BLE_CONNECTION_DATA_H
#define BLE_CONNECTION_DATA_H

// --- includes ----------------------------------------------------------------
#include <stdbool.h>
#include "../../common/common.h"
#include <zephyr.h>
#include <bluetooth/gatt.h>

// --- defines -----------------------------------------------------------------
#define BLE_MAX_CONNECTIONS 20

// --- structs -----------------------------------------------------------------
typedef struct ble_connection_data_s
{
    struct bt_conn *ble_connection_handle;
    char mac_address[MAC_ADDRESS_LENGTH];
    bool is_connected;
    uint16_t temperature_value_handle;
    uint16_t humidity_value_handle;
    uint16_t soil_moisture_value_handle;
    uint16_t light_intensity_value_handle;
    uint16_t configuration_value_handle;
    uint16_t battery_value_handle;
} ble_connection_data_t;

// --- function declarations ---------------------------------------------------
ble_connection_data_t *get_empty_ble_handle(void);
void remove_connection_data(struct bt_conn *conn);
ble_connection_data_t *get_device_by_conn_handle(struct bt_conn *conn);

struct bt_conn *get_ble_conn_handles(uint8_t index);
char *get_mac_address_by_conn_handle(struct bt_conn* conn);
#endif // BLE_CONNECTION_DATA_H
