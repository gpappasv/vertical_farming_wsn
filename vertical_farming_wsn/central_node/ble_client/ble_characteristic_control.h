#ifndef BLE_CHARACTERISTIC_CONTROL_H
#define BLE_CHARACTERISTIC_CONTROL_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <zephyr.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

// --- defines -----------------------------------------------------------------
// --- MEASUREMENT SERVICE INFO ---
// --- service indexes
#define MEASUREMENT_SERVICE_INDEX 0

// How many characteristics measurement service contains
#define MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT 4
// --- characteristic indexes
/**
 * Measurement service characteristic indexes must have values incremented by 1
 * For example, after adding another characteristic to measurement service we
 * will have a new define #define NEW_CHAR_INDEX 2 and add +1 to 
 * MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT
 */
#define TEMPERATURE_CHAR_INDEX 0
#define HUMIDITY_CHAR_INDEX 1
#define SOIL_MOISTURE_CHAR_INDEX 2
#define LIGHT_INTENSITY_CHAR_INDEX 3
// --- MEASUREMENT SERVICE INFO ---

// --- CONFIGURATION SERVICE INFO ---
// --- service indexes
#define CONFIGURE_SERVICE_INDEX 1

// How many characteristics configure service contains
#define CONFIGURE_SERVICE_CHARACTERISTIC_COUNT 2
// --- characteristic indexes
#define CONFIGURATION_CHAR_INDEX (0 + MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT)
#define BATTERY_CHAR_INDEX (1 + MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT)
// --- CONFIGURATION SERVICE INFO ---

// --- functions declarations --------------------------------------------------
void characteristic_discovery_wrapper(struct bt_conn *conn, uint8_t service_select, uint8_t char_select);
void read_characteristic_wrapper(struct bt_conn *conn, uint8_t char_select);

#endif // BLE_CHARACTERISTIC_CONTROL_H
