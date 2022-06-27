// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include "measurements_data_storage.h"
#include "measurements_fsm.h"
#include "../ble_client/ble_connection_data.h"
#include "../ble_client/ble_characteristic_control.h"
#include "../../common/common.h"

#include "logging/log.h"

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(measurements_m);

// --- static variables definitions --------------------------------------------
// measurement_data will store all measurements from each sensor node
// get_all_ble_connection_handles() function fills this array with conenction handles only
// if a connection gets invalid, measurement_data will not be updated
static measurements_data_t measurement_data[BLE_MAX_CONNECTIONS];
// mean_row_measurements will store mean measurement values for every row
static row_mean_data_t mean_row_measurements[MAX_CONFIGURATION_ID];

// --- functions definitions ---------------------------------------------------
measurements_data_t *get_measurements_data(void)
{
    return measurement_data;
}

row_mean_data_t *get_row_mean_data(void)
{
    return mean_row_measurements;
}

/**
 * @brief function to clear measurement_data, this is called
 *        every time we take measurements
 *
 */
void clear_measurement_data(void)
{
    // Clean measurement_data array
    memset(measurement_data, 0, sizeof(measurements_data_t) * BLE_MAX_CONNECTIONS);
    memset(mean_row_measurements, 0, sizeof(row_mean_data_t) * MAX_CONFIGURATION_ID);
}

/**
 * @brief Get all connection handles from connected sensor nodes and store them
 *        on measurement_data.ble_connection_handle member
 *
 */
void get_all_ble_connection_handles(void)
{
    char *mac_address;
    // store all connection handles on the local variable measurement_data
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        // TODO: make sure ble_connection_handle is null if device is not connected
        measurement_data[i].ble_connection_handle = get_ble_conn_handles(i);
        mac_address = get_mac_address_by_conn_handle(measurement_data[i].ble_connection_handle);
        if(mac_address != NULL)
        {
            memcpy(measurement_data[i].mac_address, mac_address, MAC_ADDRESS_LENGTH * sizeof(char));
        }
    }
}

/**
 * @brief Store the measured temperature on the measurement_data
 *        This function is called from the read_characteristic_cb
 *
 * @param conn Ble connection handle
 * @param measured_temperature Measured temp
 */
void set_temperature_measurement_value(struct bt_conn *conn, int32_t measured_temperature)
{
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (measurement_data[i].ble_connection_handle == conn)
        {
            measurement_data[i].ambient_temp_measurement = measured_temperature;
            k_sem_give(&read_response_sem);
            break;
        }
    }
}

/**
 * @brief Store the measured humidity on the measurement_data
 *        This function is called from the read_characteristic_cb
 *
 * @param conn Ble connection handle
 * @param measured_humidity Measured humidity
 */
void set_humidity_measurement_value(struct bt_conn *conn, int32_t measured_humidity)
{
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (measurement_data[i].ble_connection_handle == conn)
        {
            measurement_data[i].ambient_hum_measurement = measured_humidity;
            k_sem_give(&read_response_sem);
            break;
        }
    }
}

/**
 * @brief Set the soil moisture measurement on the measurement data
 *        This function is called from the read_characteristic_cb
 * 
 * @param conn 
 * @param soil_moisture 
 */
void set_soil_moisture_measurement_value(struct bt_conn *conn, int32_t soil_moisture)
{
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (measurement_data[i].ble_connection_handle == conn)
        {
            measurement_data[i].soil_moisture_measurement = soil_moisture;
            k_sem_give(&read_response_sem);
            break;
        }
    }
}

/**
 * @brief Set the light intensity measurement on the measurement data
 *        This function is called from the read_Characteristic_Cb
 * 
 * @param conn 
 * @param light_intensity 
 */
void set_light_intensity_measurement_value(struct bt_conn *conn, int32_t light_intensity)
{
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (measurement_data[i].ble_connection_handle == conn)
        {
            measurement_data[i].light_measurement = light_intensity;
            k_sem_give(&read_response_sem);
            break;
        }
    }
}

/**
 * @brief Store the configuration id on the measurement_data
 *        This function is called from the read_characteristic_cb
 *
 * @param conn Ble connection handle
 * @param configuration_id Configuration id
 */
void set_configuration_id_value(struct bt_conn *conn, uint8_t configuration_id)
{
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (measurement_data[i].ble_connection_handle == conn)
        {
            measurement_data[i].row_id = configuration_id;
            if (configuration_id > 0 && configuration_id <= MAX_CONFIGURATION_ID)
            {
                // Set row to registered
                mean_row_measurements[configuration_id - 1].is_row_registered = true;
                mean_row_measurements[configuration_id - 1].row_id = configuration_id;
            }
            else
            {
                LOG_INF("Configuration id issue: %d", configuration_id);
            }

            k_sem_give(&read_response_sem);
            break;
        }
    }
}

/**
 * @brief Store the Battery level on the measurement_data
 *        This function is called from the read_characteristic_cb
 *
 * @param conn Ble connection handle
 * @param battery_level Battery level
 */
void set_battery_level_value(struct bt_conn *conn, uint8_t battery_level)
{
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (measurement_data[i].ble_connection_handle == conn)
        {
            measurement_data[i].battery_level = battery_level;
            k_sem_give(&read_response_sem);
            break;
        }
    }
}

/**
 * @brief function to take measurements from every connected device (sensor node)
 *        It actually reads every characteristic of the measurement service
 *
 *
 * @return true if at least one measurement was taken, false if no measurements taken
 *         (false when no connected devices)
 */
bool measurements_and_device_data(void)
{
    int err;
    uint8_t measurement_taken = 0;
    // Take measurements from every connected sensor node
    for (int index = 0; index < BLE_MAX_CONNECTIONS; index++)
    {
        if (measurement_data[index].ble_connection_handle != NULL)
        {
            measurement_taken++;
            for (int char_index = 0; char_index < MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT + CONFIGURE_SERVICE_CHARACTERISTIC_COUNT; char_index++)
            {
                // Reading every characteristic value from measurement service. (As measurement service gets bigger, we will not need to change this function)
                read_characteristic_wrapper(measurement_data[index].ble_connection_handle, char_index);
                err = k_sem_take(&read_response_sem, K_MSEC(1500));
                if(err != 0)
                {
                    LOG_INF("Error in characteristics read:%d", err);
                    measurement_data[index].ble_connection_handle = NULL;
                    // Measurement was invalid, so reduce measurement counter
                    measurement_taken--;
                    break;
                }
            }
        }
    }

    return (measurement_taken > 0) ? true : false;
}

// TODO: just for debug
void print_all_measurements_and_connection_handles(void)
{
    // Take measurements from every connected sensor node
    for (int index = 0; index < BLE_MAX_CONNECTIONS; index++)
    {
        if (measurement_data[index].ble_connection_handle != NULL)
        {
            LOG_INF("-----------------");
            LOG_INF("Temperature is: %d.%d C", measurement_data[index].ambient_temp_measurement / 100, measurement_data[index].ambient_temp_measurement % 100);
            LOG_INF("Humidity is: %d.%d percent", measurement_data[index].ambient_hum_measurement / 100, measurement_data[index].ambient_hum_measurement % 100);
            LOG_INF("Soil moisture is: %d percent", measurement_data[index].soil_moisture_measurement);
            LOG_INF("Light intensity is: %d", measurement_data[index].light_measurement);
            LOG_INF("Battery level is: %d percent", measurement_data[index].battery_level);
            LOG_INF("Configuration id is: %d", measurement_data[index].row_id);
            LOG_INF("Conn handle: %d", (int)measurement_data[index].ble_connection_handle);
            LOG_INF("-----------------");
        }
    }
}