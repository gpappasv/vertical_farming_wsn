#ifndef COMMON_H
#define COMMON_H

// --- includes ----------------------------------------------------------------
#include "bluetooth/uuid.h"
#include <bluetooth/conn.h>

// --- defines -----------------------------------------------------------------
// --- ble services ---
#define MEASUREMENT_SERVICE_UUID 0x02, 0x00, 0x12, 0xac, 0x42, 0x02, 0x09, 0xb9, \
                                 0xec, 0x11, 0xf1, 0xa1, 0x28, 0x6d, 0x79, 0x6a

#define BT_UUID_MEASUREMENT_SERVICE BT_UUID_DECLARE_128(MEASUREMENT_SERVICE_UUID)

// 64c7404e-ba92-11ec-8422-0242ac120002
#define CONFIGURE_SERVICE_UUID 0x02, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x22, 0x84, \
                               0xEC, 0x11, 0x92, 0xBA, 0x4E, 0x40, 0xC7, 0x64

#define BT_UUID_CONFIGURATION BT_UUID_DECLARE_128(CONFIGURATION_ID_UUID)

// --- ble characteristics ---
#define SOIL_MOISTURE_CHARACHTERISTIC_UUID_VAL \
    BT_UUID_128_ENCODE(0x07a3e336, 0xccad, 0x11ec, 0x9d64, 0x0242ac120002)

#define BT_UUID_SOIL_MOISTURE BT_UUID_DECLARE_128(SOIL_MOISTURE_CHARACHTERISTIC_UUID_VAL)

#define LIGHT_EXPOSURE_UUID_VAL \
    BT_UUID_128_ENCODE(0x0b208f09, 0Xa0bf, 0x4d57, 0xb778, 0x38f899264e76)

#define BT_UUID_LIGHT_EXPOSURE BT_UUID_DECLARE_128(LIGHT_EXPOSURE_UUID_VAL)

// e4fd83a4-ba92-11ec-8422-0242ac120002
#define CONFIGURATION_ID_UUID 0x02, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x22, 0x84, \
                              0xEC, 0x11, 0x92, 0xBA, 0xA4, 0x83, 0xFD, 0xE4

#define BT_UUID_CONFIGURE_SERVICE BT_UUID_DECLARE_128(CONFIGURE_SERVICE_UUID)

// --- represents the max number of groups supported
#define MAX_CONFIGURATION_ID 5
#define MAC_ADDRESS_LENGTH 17

// --- enums -------------------------------------------------------------------
enum error_codes_e
{
    SUCCESS = 0x00,
    FAILURE = 0xFF
};

// --- structs -----------------------------------------------------------------
// Struct to store measurement data from each node
#pragma pack(push, 1)
typedef struct measurements_data_s
{
    // TODO: is there a better way to link each ble_connection_data_t bluetooth_devices member
    // with a measurements_data_t member? (maybe ble MAC address?) for now by connection handle
    struct bt_conn *ble_connection_handle;
    //BT_ADDR_LE_STR_LEN
    char mac_address[MAC_ADDRESS_LENGTH];
    // TODO: change measurement values to int16_t type everywhere
    int32_t ambient_temp_measurement;
    int32_t ambient_hum_measurement;
    int32_t soil_moisture_measurement;
    int32_t light_measurement;
    uint8_t battery_level; // Takes values from 0-100 (%)
    uint8_t row_id;
} measurements_data_t;
#pragma pack(pop)

// Struct to store mean measurements for each row
#pragma pack(push, 1)
typedef struct row_mean_data_s
{
    int16_t mean_row_temp;
    int16_t mean_row_humidity;
    int16_t mean_row_soil_moisture;
    int16_t mean_row_light;
    bool is_fan_active;
    bool is_watering_active;
    bool are_lights_active;
    bool is_row_registered;
    uint8_t row_id;
} row_mean_data_t;
#pragma pack(pop)

#endif // COMMON_H