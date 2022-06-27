// --- includes ----------------------------------------------------------------
#include "com_protocol.h"
#include "sys/crc.h"
#include <string.h>
#include "date_time.h"

// --- static variables definitions --------------------------------------------
static int64_t timestamp;

// --- functions definitions ---------------------------------------------------
/**
 * @brief Create a measurements data tx message object
 * 
 * @param in_buffer 
 * @param out_buffer 
 */
void create_measurements_data_tx_message(const measurements_data_t *in_buffer, message_measurement_data_t *out_buffer)
{
    // Set type and length of message
    out_buffer->len = sizeof(message_measurement_data_t);
    out_buffer->type = MESSAGE_MEASUREMENT_DATA;

    // Set the message data
    memcpy(&out_buffer->message_buffer, in_buffer, sizeof(measurements_data_t));

    // Calculate crc of the message
    out_buffer->message_crc = crc16_ansi((uint8_t*)out_buffer, sizeof(message_measurement_data_t) - sizeof(out_buffer->message_crc));
}

/**
 * @brief Create a row mean data tx message object
 * 
 * @param in_buffer 
 * @param out_buffer 
 */
void create_row_mean_data_tx_message(const row_mean_data_t *in_buffer, message_row_mean_data_t *out_buffer)
{
    // Set type and length of message
    out_buffer->len = sizeof(message_row_mean_data_t);
    out_buffer->type = MESSAGE_ROW_MEAN_DATA;

    // Set the message data
    memcpy(&out_buffer->message_buffer, in_buffer, sizeof(row_mean_data_t));

    // Calculate crc of the message
    out_buffer->message_crc = crc16_ansi((uint8_t*)out_buffer, sizeof(message_row_mean_data_t) - sizeof(out_buffer->message_crc));
}

/**
 * @brief Create a operation result tx message object
 * 
 * @param out_buffer 
 * @param status 
 */
void create_operation_result_tx_message(message_operation_result_t *out_buffer, uint8_t status)
{
    // Set type and length of message
    out_buffer->len = sizeof(message_operation_result_t);
    out_buffer->type = MESSAGE_OPERATION_RESULT;

    // Set the message data
    out_buffer->operation_result = status;

    // Calculate crc of the message
    out_buffer->message_crc = crc16_ansi((uint8_t*)out_buffer, sizeof(message_operation_result_t) - sizeof(out_buffer->message_crc));
}

/**
 * @brief Create a ready for cloud tx message object
 * 
 * @param out_buffer 
 */
void create_ready_for_cloud_tx_message(message_ready_for_cloud_t *out_buffer)
{
    // Set type and length of message
    out_buffer->len = sizeof(message_ready_for_cloud_t);
    out_buffer->type = MESSAGE_READY_FOR_CLOUD;

    // Calculate crc of the message
    out_buffer->message_crc = crc16_ansi((uint8_t*)out_buffer, sizeof(message_ready_for_cloud_t) - sizeof(out_buffer->message_crc));
}

/**
 * @brief Create a coap row mean data message object
 * 
 * @param in_buffer 
 * @param out_buffer 
 */
void create_coap_row_mean_data_message(const row_mean_data_t* in_buffer, message_coap_row_mean_data_t *out_buffer)
{
    // Set type and length of message
    out_buffer->len = sizeof(message_coap_row_mean_data_t);
    out_buffer->type = MESSAGE_COAP_ROW_MEAN_DATA;

    // Set the message data
    out_buffer->mean_hum = in_buffer->mean_row_humidity;
    out_buffer->mean_temp = in_buffer->mean_row_temp;
    out_buffer->mean_soil_moisture = in_buffer->mean_row_soil_moisture;
    out_buffer->mean_light_intensity = in_buffer->mean_row_light;
    out_buffer->is_fan_active = in_buffer->is_fan_active;
    out_buffer->is_water_on = in_buffer->is_watering_active;
    out_buffer->is_light_on = in_buffer->are_lights_active;
    out_buffer->row_id = in_buffer->row_id;

    // Get calendar time
    date_time_now(&timestamp);
    // Add 3 hours to Unix time to match our timezone offset
    timestamp += 10800000;
    // Timestamp is in unix time
    out_buffer->timestamp = timestamp;

    // Calculate crc of the message
    out_buffer->message_crc = crc16_ansi((uint8_t*)out_buffer, sizeof(message_coap_row_mean_data_t) - sizeof(out_buffer->message_crc));
}
