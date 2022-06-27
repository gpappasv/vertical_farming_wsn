#ifndef COM_PROTOCOL_H
#define COM_PROTOCOL_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include "../common.h"

// --- defines -----------------------------------------------------------------
// second byte of every message will be the message length
#define MSG_LENGTH_POSITION 1
#define MSG_TYPE_POSITION 0

// --- Communication 9160 - 52840 ---
#define MESSAGE_MEASUREMENT_DATA 0xA1
#define MESSAGE_ROW_MEAN_DATA 0xA2
#define MESSAGE_OPERATION_RESULT 0xFD
#define MESSAGE_READY_FOR_CLOUD 0xC0

// --- COAP messages ---
#define MESSAGE_COAP_ROW_MEAN_DATA 0xB1
#define MESSAGE_COAP_ROW_CONTROL_USER_DATA 0xB2
#define MESSAGE_COAP_ROW_THRESHOLDS_USER_DATA 0xB3

// --- enums -------------------------------------------------------------------
// --- MESSAGE_OPERATION_RESULT ---
enum message_operation_resuls_e
{
    MEASUREMENTS_DATA_MSG_SUCCESS = 0x01,
    MEASUREMENTS_DATA_MSG_CRC_ERROR = 0x02,
    MEASUREMENTS_DATA_MSG_FAIL = 0x04,
    ROW_MEAN_DATA_MSG_SUCCESS = 0x08,
    ROW_MEAN_DATA_MSG_CRC_ERROR = 0x10,
    ROW_MEAN_DATA_MSG_FAIL = 0x20,
    SEND_TO_CLOUD_MSG_CRC_ERROR = 0x40,
    SEND_TO_CLOUD_MSG_SUCCESS = 0x80,
    GENERIC_ERROR = 0xFF
};

// --- structs -----------------------------------------------------------------
// --- MESSAGE_MEASUREMENT_DATA ---
#pragma pack(push, 1)
typedef struct message_measurement_data_s
{
    uint8_t type;
    uint8_t len;
    measurements_data_t message_buffer;
    uint16_t message_crc;
} message_measurement_data_t;
#pragma pack(pop)

// --- MESSAGE_ROW_MEAN_DATA ---
#pragma pack(push, 1)
typedef struct message_row_mean_data_s
{
    uint8_t type;
    uint8_t len;
    row_mean_data_t message_buffer;
    uint16_t message_crc;
} message_row_mean_data_t;
#pragma pack(pop)

// --- MESSAGE_OPERATION_RESULT ---
#pragma pack(push, 1)
typedef struct message_operation_result_s
{
    uint8_t type;
    uint8_t len;
    uint16_t operation_result;
    uint16_t message_crc;
} message_operation_result_t;
#pragma pack(pop)

// --- MESSAGE_READY_FOR_CLOUD ---
#pragma pack(push, 1)
typedef struct message_ready_for_cloud_s
{
    uint8_t type;
    uint8_t len;
    uint16_t message_crc;
} message_ready_for_cloud_t;
#pragma pack(pop)

// --- MESSAGE_COAP_ROW_MEAN_DATA ---
#pragma pack(push, 1)
typedef struct message_coap_row_mean_data_s
{
    uint8_t type;
    uint8_t len;
    int16_t mean_temp;
    uint16_t mean_hum;
    uint16_t mean_soil_moisture;
    uint16_t mean_light_intensity;
    uint8_t  row_id;
    int64_t  timestamp;
    bool     is_light_on;
    bool     is_water_on;
    bool     is_fan_active;
    // TODO: currently unused
    uint16_t message_crc;
} message_coap_row_mean_data_t;
#pragma pack(pop)

// --- MESSAGE_COAP_ROW_CONTROL_USER_DATA ---
#pragma pack(push, 1)
typedef struct message_coap_row_control_user_data_s
{
    uint8_t type;
    uint8_t len;
    uint8_t  row_id;
    bool     is_automatic_control;
    bool     is_light_on;
    bool     is_water_on;
    bool     is_fan_active;
    // TODO: currently unused
    uint16_t message_crc;
} message_coap_row_control_user_data_t;
#pragma pack(pop)

// --- MESSAGE_COAP_ROW_THRESHOLDS_USER_DATA ---
#pragma pack(push, 1)
typedef struct message_coap_row_thresholds_user_data_s
{
    uint8_t type;
    uint8_t len;
    uint8_t  row_id;
    int16_t  temp_threshold;
    uint16_t  humidity_threshold;
    uint16_t  light_threshold;
    uint16_t  soil_moisture_threshold;
    // TODO: currently unused
    uint16_t message_crc;
} message_coap_row_thresholds_user_data_t;
#pragma pack(pop)

// --- functions declarations --------------------------------------------------
void create_measurements_data_tx_message(const measurements_data_t *in_buffer, message_measurement_data_t *out_buffer);
void create_row_mean_data_tx_message(const row_mean_data_t *in_buffer, message_row_mean_data_t *out_buffer);
void create_operation_result_tx_message(message_operation_result_t *out_buffer, uint8_t status);
void create_ready_for_cloud_tx_message(message_ready_for_cloud_t *out_buffer);
void create_coap_row_mean_data_message(const row_mean_data_t* in_buffer, message_coap_row_mean_data_t *out_buffer);

#endif // COM_PROTOCOL_H