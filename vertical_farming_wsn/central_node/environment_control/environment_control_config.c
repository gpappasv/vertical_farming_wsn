// --- includes ----------------------------------------------------------------
#include "environment_control_config.h"
#include "../flash_system/flash_system.h"
#include "../../common/common.h"
#include <logging/log.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(environment_control_m);

// --- defines -----------------------------------------------------------------

// --- static variables definitions --------------------------------------------
// On this object all the necessary information about each row is stored
// for example we store for each row if the lights should  be on on manual mode
// or if automatic mode on a row is enabled/disabled
// also we store information about the mean measurements on each row
// All of this data are evaluated by environment_control_fsm which in turn will
// call the necessary functions in order to turn on/off the fan/light/water for
// each row
static row_status_t row_control_config[MAX_CONFIGURATION_ID];
// It is initialized inside of initialize_row_control_configuration
static struct k_work store_row_config_params_work;
// index to know which row id params changed
static uint8_t row_id_params_changed;

// --- static functions declarations ------------------------------------------
static void store_row_config_params_handler(struct k_work *work);
static void store_row_config_params_in_nvs(uint8_t row_id);

// --- static functions definitions --------------------------------------------
/**
 * @brief Store row config params work handler. It is called in worqueues in order
 *        to store the new row control config params received by the user
 *
 * @param work
 */
static void store_row_config_params_handler(struct k_work *work)
{
    store_row_config_params_in_nvs(row_id_params_changed);
}

/**
 * @brief Function to store the updated row control config params in flash
 *        It is triggered every time user updates those parameters
 *
 * @param row_id
 */
static void store_row_config_params_in_nvs(uint8_t row_id)
{
    int err;

    err = nvs_write(get_file_system_handle(), row_id, &row_control_config[row_id].row_control, sizeof(row_control_config[row_id].row_control));
    if (err < 0)
    {
        LOG_INF("NVS write failed (err: %d)", err);
    }
    else
    {
        LOG_INF("Stored row config params for row id: %d", row_id + 1);
    }
}

// --- functions definitions ---------------------------------------------------
/**
 * @brief function to initialize row_control object. Loads default environment control
 *        parameters.
 *        row_control_config[].row_control is stored in flash. those are the parameters
 *        set by the user in order to control each row
 *
 */
void initialize_row_control_configuration(void)
{
    int err;
    row_control_t row_control_config_params;
    // Initialize work item to store new row config params in flash
    k_work_init(&store_row_config_params_work, store_row_config_params_handler);
    // Check if default row control parameters exist already in flash
    for (int row_id = 0; row_id < MAX_CONFIGURATION_ID; row_id++)
    {
        // Check if row control config params exist already in flash
        if (nvs_read(get_file_system_handle(), row_id, &row_control_config_params, sizeof(row_control_config_params)) != sizeof(row_control_config_params))
        {
            // load default thresholds - control parameters
            row_control_config[row_id].row_control.temp_threshold = DEFAULT_TEMPERATURE_THRESHOLD;
            row_control_config[row_id].row_control.humidity_threshold = DEFAULT_HUMIDITY_THRESHOLD;
            row_control_config[row_id].row_control.light_threshold = DEFAULT_LIGHT_EXPOSURE_THRESHOLD;
            row_control_config[row_id].row_control.soil_moisture_threshold = DEFAULT_SOIL_MOISTURE_THRESHOLD;
            row_control_config[row_id].row_control.fan_switch = false;
            row_control_config[row_id].row_control.light_switch = false;
            row_control_config[row_id].row_control.water_switch = false;
            // Automatic control is enabled by default -> User must set it to false
            row_control_config[row_id].row_control.automatic_control = true;
            // Store those parameters in flash
            err = nvs_write(get_file_system_handle(), row_id, &row_control_config[row_id].row_control, sizeof(row_control_config[row_id].row_control));
            if (err < 0)
            {
                LOG_INF("NVS write failed (err: %d)", err);
            }
        }
        else
        {
            // Load row config params from flash
            memcpy(&row_control_config[row_id].row_control, &row_control_config_params, sizeof(row_control_config_params));
            LOG_INF("Loaded default row control config params from flash, row_id = %d", row_id + 1);
        }
        // Row id gets values from 1 to MAX_CONFIGURATION_ID
        row_control_config[row_id].row_id = row_id + 1;
        row_control_config[row_id].row_hum = 0;
        row_control_config[row_id].row_light_exposure = 0;
        row_control_config[row_id].row_soil_moisture = 0;
        row_control_config[row_id].row_temp = 0;
        // By default, no registered row. This flag will be set to true if a measurement
        // is taken from a sensor node existing on the corresponding row id
        row_control_config[row_id].row_registered = false;
    }
}

/**
 * @brief Set the row current temperature object
 *
 * @param value temperature value
 * @param index This parameter is the indexing of row_control_config array,
 *               not the real row id
 */
void set_row_current_temperature(int32_t value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_temp = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row current temperature object
 *
 * @param index
 * @return int32_t
 */
int32_t get_row_current_temperature(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        // temperature is like 20.32 C -> 2032 we lose precision but its ok
        // TODO: fix the precision for all gets of this source file
        return row_control_config[index].row_temp / 100;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return 0;
}

/**
 * @brief Set the row current humidity object
 *
 * @param value
 * @param index
 */
void set_row_current_humidity(int32_t value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_hum = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row current humidity object
 *
 * @param index
 * @return int32_t
 */
int32_t get_row_current_humidity(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_hum  / 100;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return 0;
}

/**
 * @brief Set the row current soil moisture object
 *
 * @param value
 * @param index
 */
void set_row_current_soil_moisture(int32_t value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_soil_moisture = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row current soil moisture object
 *
 * @param index
 * @return int32_t
 */
int32_t get_row_current_soil_moisture(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_soil_moisture / 100;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return 0;
}

/**
 * @brief Set the row current light exposure object
 *
 * @param value
 * @param index
 */
void set_row_current_light_exposure(int32_t value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_light_exposure = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row current light exposure object
 *
 * @param index
 * @return int32_t
 */
int32_t get_row_current_light_exposure(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_light_exposure / 100;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return 0;
}

/**
 * @brief Set the row registered object
 *
 * @param index
 */
void set_row_registered(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_registered = true;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Set the row temp threshold object
 *
 * @param value
 * @param index
 */
void set_row_temp_threshold(int32_t value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_control.temp_threshold = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row temp threshold object
 *
 * @param index
 * @return int32_t
 */
int32_t get_row_temp_threshold(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_control.temp_threshold;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return 0;
}

/**
 * @brief Set the row hum threshold object
 *
 * @param value
 * @param index
 */
void set_row_hum_threshold(int32_t value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_control.humidity_threshold = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row hum threshold object
 *
 * @param index
 * @return int32_t
 */
int32_t get_row_hum_threshold(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_control.humidity_threshold;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return 0;
}

/**
 * @brief Set the row light threshold object
 *
 * @param value
 * @param index
 */
void set_row_light_threshold(int32_t value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_control.light_threshold = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row light threshold object
 *
 * @param index
 * @return int32_t
 */
int32_t get_row_light_threshold(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_control.light_threshold;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return 0;
}

/**
 * @brief Set the row soil moisture threshold object
 *
 * @param value
 * @param index
 */
void set_row_soil_moisture_threshold(int32_t value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_control.soil_moisture_threshold = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row soil moisture threshold object
 *
 * @param index
 * @return int32_t
 */
int32_t get_row_soil_moisture_threshold(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_control.soil_moisture_threshold;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return 0;
}

/**
 * @brief Set the row water switch object
 *
 * @param value
 * @param index
 */
void set_row_water_switch(bool value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_control.water_switch = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row water switch object
 *
 * @param index
 * @return true
 * @return false
 */
bool get_row_water_switch(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_control.water_switch;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return false;
}

/**
 * @brief Set the row fan switch object
 *
 * @param value
 * @param index
 */
void set_row_fan_switch(bool value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_control.fan_switch = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row fan switch object
 *
 * @param index
 * @return true
 * @return false
 */
bool get_row_fan_switch(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_control.fan_switch;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return false;
}

/**
 * @brief Set the row light switch object
 *
 * @param value
 * @param index
 */
void set_row_light_switch(bool value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_control.light_switch = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row light switch object
 *
 * @param index
 * @return true
 * @return false
 */
bool get_row_light_switch(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_control.light_switch;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return false;
}

/**
 * @brief Set the row automatic control object
 *
 * @param value
 * @param index
 */
void set_row_automatic_control(bool value, uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_control_config[index].row_control.automatic_control = value;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }
}

/**
 * @brief Get the row automatic control object
 *
 * @param index
 * @return true
 * @return false
 */
bool get_row_automatic_control(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_control.automatic_control;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return false;
}

/**
 * @brief Get the row registered object
 *
 * @param index
 * @return true
 * @return false
 */
bool get_row_registered(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        return row_control_config[index].row_registered;
    }
    else
    {
        LOG_INF("Wrong row control config indexing %d", index);
    }

    return false;
}

/**
 * @brief Reset row status
 *
 * @param index this is the indexing of the row control config array and index = row_id - 1
 */
void reset_row_status(uint8_t index)
{
    row_control_config[index].row_hum = 0;
    row_control_config[index].row_light_exposure = 0;
    row_control_config[index].row_soil_moisture = 0;
    row_control_config[index].row_temp = 0;
    row_control_config[index].row_registered = false;
}

/**
 * @brief Function to store new row config params in flash
 *
 * @param index
 */
void update_row_control_config_params_in_nvs(uint8_t index)
{
    if (index < MAX_CONFIGURATION_ID)
    {
        row_id_params_changed = index;
        k_work_submit(&store_row_config_params_work);
    }
    else
    {
        LOG_INF("Wrong row id index in update_row_control_config_params_in_nvs() %d", index);
    }
}
