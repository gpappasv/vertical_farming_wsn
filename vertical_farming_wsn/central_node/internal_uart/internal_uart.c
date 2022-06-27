// --- includes ----------------------------------------------------------------
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/uart.h>
#include <string.h>
#include <stdint.h>
#include <logging/log.h>
#include "internal_uart.h"
#include "message_parsing.h"
#include "../../common/com_protocol/com_protocol.h"

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(internal_uart_m);

// --- defines -----------------------------------------------------------------
#define FIRST_BYTE_ARRIVED 1
#define SECOND_BYTE_ARRIVED 2
#define MESSAGE_LENGTH_UNCONFIGURED 0
#define RESET_VALUE 0
#define UART_SEND_SEM_TIMEOUT_MS 1500

// --- static variables definitions --------------------------------------------
static const struct device *uart_dev; //Figure out why you should/should not use static
static uint8_t uart_buf[UART_BUFFER];
static int data_length = 0;
struct k_sem uart_send_sem;

// --- static functions definitions --------------------------------------------
static void uart_cb(const struct device *x, void *user_data);

// --- static functions declarations -------------------------------------------
static void uart_cb(const struct device *x, void *user_data)
{
    static uint8_t message_length = RESET_VALUE;
    uart_irq_update(x);

    if (uart_irq_rx_ready(x))
    {
        data_length += uart_fifo_read(x, &uart_buf[data_length], sizeof(uart_buf));
        // Evaluate message type
        if(data_length == 1)
        {
            // If message type is not valid, ignore
            if(!is_message_type_valid(uart_buf[MSG_TYPE_POSITION]))
            {
                // reset message and buffer
                data_length = RESET_VALUE;
                memset(uart_buf, RESET_VALUE, UART_BUFFER * sizeof(uint8_t));
                LOG_INF("Invalid message type: %d", uart_buf[MSG_TYPE_POSITION]);
            }
        }

        // if the "message length" byte arrived and message_length is not configured
        // yet (message_length == 0), then store the message length value
        if(data_length >= SECOND_BYTE_ARRIVED && message_length == MESSAGE_LENGTH_UNCONFIGURED)
        {
            message_length = uart_buf[MSG_LENGTH_POSITION];
            LOG_INF("Message length is: %d", message_length);
        }

        // If message_length is configured check for message end
        if(message_length != MESSAGE_LENGTH_UNCONFIGURED)
        {
            // reached the end of the message
            if(data_length == message_length)
            {
                // parse message
                process_rx_message(uart_buf);
                // reset data_length and buffer
                data_length = RESET_VALUE;
                message_length = RESET_VALUE;
                memset(uart_buf, RESET_VALUE, UART_BUFFER * sizeof(uint8_t));
            }
        }
    }
}

// --- functions definitions ---------------------------------------------------
/**
 * @brief Initialize internal uart
 * 
 */
void internal_uart_init(void)
{
    // get uart device
    uart_dev = device_get_binding("UART_1");

    // log in case of uart device error
    if (!uart_dev)
    {
        LOG_INF("Could not get UART 1");
    }

    // Set uart interrupt cb function
    uart_irq_callback_set(uart_dev, uart_cb);
    uart_irq_rx_enable(uart_dev);
    // Init sem to lock send function
    k_sem_init(&uart_send_sem, 1, 1);
}

/**
 * @brief Send data via internal uart
 * 
 * @param buf Buffer with data to send
 * @param size Size of buffer
 * @return int 
 */
int internal_uart_send_data(const uint8_t *buf, size_t size)
{
    k_sem_take(&uart_send_sem, K_MSEC(UART_SEND_SEM_TIMEOUT_MS));
    //printk("size of output_buffer: %d\n", size);
    if (size == 0)
    {
        k_sem_give(&uart_send_sem);
        // Zero size
        return FAILURE;
    }

    // poll out the data
    for (int i = 0; i < size; i++)
    {
        uart_poll_out(uart_dev, buf[i]);
    }

    return SUCCESS;
}