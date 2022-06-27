// --- includes ----------------------------------------------------------------
#include "coap_message_parsing.h"
#include "../../common/com_protocol/com_protocol.h"
#include "../internal_uart/internal_uart.h"
#include <logging/log.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(coap_m);

// --- functions definitions ---------------------------------------------------
/**
 * @brief Function to process the message received from coap server
 *        Takes actions based on the content of the message
 *
 * @param rx_buf
 * @return uint8_t
 */
uint8_t process_coap_rx_message(const uint8_t *rx_buf)
{
    message_coap_row_control_user_data_t *p_msg_coap_control_user_data;
    message_coap_row_thresholds_user_data_t *p_msg_coap_thresholds_user_data;
    uint8_t ret = GENERIC_ERROR;
    if (rx_buf != NULL)
    {
        switch (rx_buf[MSG_TYPE_POSITION])
        {
        // TODO: calculate the crc of the buffer
        case MESSAGE_COAP_ROW_CONTROL_USER_DATA:
            p_msg_coap_control_user_data = (message_coap_row_control_user_data_t *)rx_buf;
            // TODO: here, validate the crc
            // Messages received from coap server should be forwarded to 52840
            internal_uart_send_data(rx_buf, p_msg_coap_control_user_data->len);
            ret = SUCCESS;
            break;
        // TODO: calculate the crc of the buffer
        case MESSAGE_COAP_ROW_THRESHOLDS_USER_DATA:
            p_msg_coap_thresholds_user_data = (message_coap_row_thresholds_user_data_t *)rx_buf;
            // TODO: here, validate the crc
            // Messages received from coap server should be forwarded to 52840
            internal_uart_send_data(rx_buf, p_msg_coap_thresholds_user_data->len);
            ret = SUCCESS;
            break;
        default:
            LOG_INF("Invalid coap rx message type");
            break;
        }
    }

    return ret;
}