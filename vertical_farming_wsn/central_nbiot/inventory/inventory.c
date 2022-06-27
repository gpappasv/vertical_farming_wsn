// --- includes ----------------------------------------------------------------
#include "inventory.h"
#include "../../common/common.h"
#include <stdint.h>
#include <bluetooth/conn.h>
#include <logging/log.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(inventory_m);

// --- defines -----------------------------------------------------------------
#define BLE_MAX_CONNECTIONS 20

// --- static variables definitions --------------------------------------------
// Buffer to store measurements from sensor nodes. Size of the buffer ==
// max simultaneous sensor node connections
static measurements_data_t measurements_inventory[BLE_MAX_CONNECTIONS];
static uint8_t measurements_inventory_fill_index = 0;
// Buffer to store the mean measurements for each row
// The indexing for this buffer is: row id = 1 -> row_mean_data_inventory[0]
static row_mean_data_t row_mean_data_inventory[MAX_CONFIGURATION_ID];

// --- functions definitions ---------------------------------------------------
/**
 * @brief Function to store the measurement data received from 52840.
 *        This function fills measurements inventory. If the inventory is full,
 *        we will just log that it is full. This inventory should be erased
 *        after sending it to cloud
 *        52840 can send at most BLE_MAX_CONNECTIONS measurements messages, and
 *        after that, 52840 should notify 9160 to send the data to cloud and reset
 *        the inventory buffer
 *
 * @param msg_to_store
 * @return error code
 */
uint8_t store_measurement_message(message_measurement_data_t *msg_to_store)
{
    uint8_t ret = GENERIC_ERROR;
    if (measurements_inventory_fill_index < BLE_MAX_CONNECTIONS)
    {
        memcpy(&measurements_inventory[measurements_inventory_fill_index], &msg_to_store->message_buffer, sizeof(measurements_data_t));
        measurements_inventory_fill_index++;
        ret = SUCCESS;
    }
    else
    {
        LOG_INF("Measurements inventory is full %d", measurements_inventory_fill_index);
        ret = GENERIC_ERROR;
    }

    return ret;
}

/**
 * @brief Function to store the row mean data received from 52840.
 *        This function fills row_mean_data_inventory. If the inventory is full,
 *        we will just log that it is full. This inventory should be erased
 *        after sending it to cloud
 *        52840 can send at most BLE_MAX_CONNECTIONS row mean data messages, and
 *        after that, 52840 should notify 9160 to send the data to cloud and reset
 *        the inventory buffer
 *
 * @param msg_to_store
 * @return error code
 */
uint8_t store_row_mean_data_message(message_row_mean_data_t *msg_to_store)
{
    uint8_t ret = FAILURE;

    if (msg_to_store->message_buffer.row_id <= MAX_CONFIGURATION_ID || msg_to_store->message_buffer.row_id > 0)
    {
        memcpy(&row_mean_data_inventory[msg_to_store->message_buffer.row_id - 1], &msg_to_store->message_buffer, sizeof(row_mean_data_t));
        ret = SUCCESS;
    }
    else
    {
        LOG_INF("Row id is wrong: %d", msg_to_store->message_buffer.row_id);
        ret = FAILURE;
    }

    return ret;
}

/**
 * @brief Get the row mean data inventory object
 * 
 * @return row_mean_data_t* 
 */
row_mean_data_t *get_row_mean_data_inventory(void)
{
    return row_mean_data_inventory;
}

/**
 * @brief Get the measurements data inventory object
 * 
 * @return measurements_data_t* 
 */
measurements_data_t *get_measurements_data_inventory(void)
{
    return measurements_inventory;
}

/**
 * @brief Reset measurements inventory
 *
 */
void reset_measurements_inventory(void)
{
    memset(measurements_inventory, 0, sizeof(measurements_data_t) * BLE_MAX_CONNECTIONS);
    measurements_inventory_fill_index = 0;
}

/**
 * @brief Reset row mean data inventory
 *
 */
void reset_row_mean_data_inventory(void)
{
    memset(row_mean_data_inventory, 0, sizeof(row_mean_data_t) * MAX_CONFIGURATION_ID);
}