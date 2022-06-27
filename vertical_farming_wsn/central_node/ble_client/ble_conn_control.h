#ifndef BLE_CONN_CONTROL_H
#define BLE_CONN_CONTROL_H

// --- includes ----------------------------------------------------------------
#include "ble_connection_data.h"

#include <bluetooth/uuid.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

// --- defines -----------------------------------------------------------------
#define BT_CONN_LE_CREATE_LOW_POWER \
	BT_CONN_LE_CREATE_PARAM(BT_CONN_LE_OPT_NONE, \
				BT_GAP_SCAN_SLOW_INTERVAL_2, \
				BT_GAP_SCAN_SLOW_WINDOW_2)

// --- function declarations ---------------------------------------------------
ble_connection_data_t *start_scan(const struct bt_le_scan_param *scan_parameters);
void bt_ready(int err);

#endif // BLE_CONN_CONTROL_H
