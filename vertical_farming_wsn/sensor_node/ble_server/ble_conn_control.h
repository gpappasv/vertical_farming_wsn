#ifndef BLE_CONN_CONTROL_H
#define BLE_CONN_CONTROL_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

// --- defines -----------------------------------------------------------------
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// --- functions declarations --------------------------------------------------
void ble_init(void);
struct bt_conn* get_ble_connection(void);
void start_configure_state_adv(void);
void start_operating_state_adv(void);

#endif // BLE_CONN_CONTROL_H