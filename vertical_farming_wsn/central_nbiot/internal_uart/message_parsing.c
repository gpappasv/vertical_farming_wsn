// --- includes ----------------------------------------------------------------
#include "message_parsing.h"
#include "../inventory/inventory.h"
#include "../../common/com_protocol/com_protocol.h"
#include <logging/log.h>
#include "sys/crc.h"
#include "../coap_client/coap_client.h"
#include "internal_uart.h"

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(internal_uart_m);

// --- extern variables declarations -------------------------------------------
extern struct k_event uart_communication_events;
extern struct k_sem uart_send_sem;

// --- functions definitions ---------------------------------------------------
/**
 * @brief Process internal uart received message based on the message type
 *
 * @param rx_buf buffer containing the message
 */
uint8_t process_rx_message(uint8_t *rx_buf)
{
    message_measurement_data_t *p_msg_measurement_data;
    message_row_mean_data_t *p_msg_row_mean_data;
    message_operation_result_t *p_msg_op_result;
    message_ready_for_cloud_t *p_msg_ready_for_cloud;

    uint16_t calculated_crc;
    uint8_t ret = GENERIC_ERROR;

    switch (rx_buf[MSG_TYPE_POSITION])
    {
    // TODO: could clean this message up, keep only battery level and mac address
    // or keep it as is as it is only internal information and create smaller messages
    // for coap
    // Only send data from 52840 for nodes that need to update the battery level on server
    case MESSAGE_MEASUREMENT_DATA:
        p_msg_measurement_data = (message_measurement_data_t *)rx_buf;
        // --- calculate crc of the message
        calculated_crc = crc16_ansi((uint8_t *)p_msg_measurement_data, sizeof(message_measurement_data_t) - sizeof(p_msg_measurement_data->message_crc));

        // Check message crc
        if (calculated_crc != p_msg_measurement_data->message_crc)
        {
            // TODO: crc error
            LOG_INF("CRC error, calc crc = %d, received crc = %d", calculated_crc, p_msg_measurement_data->message_crc);
            ret = MEASUREMENTS_DATA_MSG_CRC_ERROR;
        }
        else
        {
            // Store the measurement data to inventory
            // Measurement data contains info about who sent the data
            ret = store_measurement_message(p_msg_measurement_data) == SUCCESS ? MEASUREMENTS_DATA_MSG_SUCCESS : MEASUREMENTS_DATA_MSG_FAIL;
            LOG_INF("Received measurement data message from 52840");
        }

        // Notify op result reply thread to reply to the message
        k_event_post(&uart_communication_events, MESSAGE_TO_REPLY_EVT);
        break;
    case MESSAGE_ROW_MEAN_DATA:
        p_msg_row_mean_data = (message_row_mean_data_t *)rx_buf;
        // --- calculate crc of the message
        calculated_crc = crc16_ansi((uint8_t *)p_msg_row_mean_data, sizeof(message_row_mean_data_t) - sizeof(p_msg_row_mean_data->message_crc));

        // Check message crc
        if (calculated_crc != p_msg_row_mean_data->message_crc)
        {
            // TODO: crc error
            LOG_INF("CRC error, calc crc = %d, received crc = %d", calculated_crc, p_msg_row_mean_data->message_crc);
            ret = ROW_MEAN_DATA_MSG_CRC_ERROR;
        }
        else
        {
            // Store the row mean data to inventory
            ret = store_row_mean_data_message(p_msg_row_mean_data) == SUCCESS ? ROW_MEAN_DATA_MSG_SUCCESS : ROW_MEAN_DATA_MSG_FAIL;
            LOG_INF("Received row mean data message from 52840");
        }
        // Notify op result reply thread to reply to the message
        k_event_post(&uart_communication_events, MESSAGE_TO_REPLY_EVT);
        break;
    case MESSAGE_READY_FOR_CLOUD:
        p_msg_ready_for_cloud = (message_ready_for_cloud_t *)rx_buf;
        // --- calculate crc of the message
        calculated_crc = crc16_ansi((uint8_t *)p_msg_ready_for_cloud, sizeof(message_ready_for_cloud_t) - sizeof(p_msg_ready_for_cloud->message_crc));

        // Check message crc
        if (calculated_crc != p_msg_ready_for_cloud->message_crc)
        {
            // TODO: crc error
            LOG_INF("CRC error, calc crc = %d, received crc = %d", calculated_crc, p_msg_ready_for_cloud->message_crc);
            ret = SEND_TO_CLOUD_MSG_CRC_ERROR;
        }
        else
        {
            ret = SEND_TO_CLOUD_MSG_SUCCESS;
        }

        // Notify op result reply thread to reply to the message
        k_event_post(&uart_communication_events, MESSAGE_TO_REPLY_EVT);
        k_event_post(&uart_communication_events, COAP_ROW_DATA_TO_SERVER_EVT);

        break;
    case MESSAGE_OPERATION_RESULT:
        p_msg_op_result = (message_operation_result_t *)rx_buf;
        // TODO: Parse the message and store it
        break;
    default:
        break;
    }

    return ret;
}

/**
 * @brief evaluate received message
 *
 * @param type
 * @return true
 * @return false
 */
bool is_message_type_valid(uint8_t type)
{
    bool ret;

    // Check if message type is valid
    switch(type)
    {
        case MESSAGE_MEASUREMENT_DATA:
        case MESSAGE_ROW_MEAN_DATA:
            ret = true;
            break;
        case MESSAGE_OPERATION_RESULT:
            ret = true;
            // Receiving operation result message, means we can go on with next send
            // Todo, parse operation result message for errors and handle/log them
            LOG_INF("Received op result msg from 52840");
            k_sem_give(&uart_send_sem);
            break;
        case MESSAGE_READY_FOR_CLOUD:
            ret = true;
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}