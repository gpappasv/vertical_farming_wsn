#ifndef BLE_MEASUREMENT_SERVICE
#define BLE_MEASUREMENT_SERVICE

// --- includes ----------------------------------------------------------------
#include <stdint.h>

#include <drivers/sensor.h>

#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

// --- functions declarations --------------------------------------------------
void measurement_ble_send(void *data, uint16_t len, 
                          const struct bt_uuid *char_uuid, uint8_t attr_index);

#endif // BLE_MEASUREMENT_SERVICE