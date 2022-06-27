// --- includes ----------------------------------------------------------------
#include "temperature_humidity_sensor.h"
#include <logging/log.h>
#include <pm/device.h>
// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(temperature_humidity_sensor_m);

// --- static variables definitions --------------------------------------------
// Struct variable to store temperature and humidity measurements
static bme_measurements_t temp_hum_measurements = {0};

// --- functions definitions ---------------------------------------------------
/**
 * @brief Function to check if bme280 device is ready for use
 * 
 * @return true if ready for use
 * @return false if not ready
 */
bool init_bme280(void)
{
    const static struct device *bme_280_device = DEVICE_DT_GET_ANY(bosch_bme280);
    if(!device_is_ready(bme_280_device))
    {
        LOG_INF("BME280 device not ready during init");
        return false;
    }

    return true;
}

// Update temp_hum_measurements struct with latest measurements
// TODO: maybe add error code?
void measure_temperature(void)
{
    int err;
    // Resume spi
    const static struct device *bme_spi_device = DEVICE_DT_GET_ANY(nordic_nrf_spim);
    pm_device_action_run(bme_spi_device, PM_DEVICE_ACTION_RESUME);

    const static struct device *bme_280_device = DEVICE_DT_GET_ANY(bosch_bme280);

    if ((bme_280_device == NULL) || !device_is_ready(bme_280_device))
    {
        // TODO: maybe add a define to enable and disable debug prints
        LOG_INF("Check bme280 sensor: not available\n");
        return;
    }

    // Take measurements and store them on sensor buffer
    err = sensor_sample_fetch_chan(bme_280_device, SENSOR_CHAN_AMBIENT_TEMP);
    if (err != 0)
    {
        LOG_INF("Error in fetching sample %d", err);
    }

    // Update temperature value
    err = sensor_channel_get(bme_280_device, SENSOR_CHAN_AMBIENT_TEMP, &temp_hum_measurements.temperature);
    if (err != 0)
    {
        LOG_INF("Error in fetching sample %d", err);
    }
    // Disable spi to save power
    pm_device_action_run(bme_spi_device, PM_DEVICE_ACTION_SUSPEND);
}

// Update temp_hum_measurements struct with latest measurements
// TODO: maybe add error code?
void measure_humidity(void)
{
    int err;
    // Resume spi
    const static struct device *bme_spi_device = DEVICE_DT_GET_ANY(nordic_nrf_spim);
    pm_device_action_run(bme_spi_device, PM_DEVICE_ACTION_RESUME);

    const static struct device *bme_280_device = DEVICE_DT_GET_ANY(bosch_bme280);

    if ((bme_280_device == NULL) || (!device_is_ready(bme_280_device)))
    {
        // TODO: maybe add a define to enable and disable debug prints
        LOG_INF("Check bme280 sensor: not available\n");
    }

    // Take measurements and store them on sensor buffer
    err = sensor_sample_fetch_chan(bme_280_device, SENSOR_CHAN_HUMIDITY);
    if (err != 0)
    {
        LOG_INF("Error in fetching sample %d", err);
    }

    // Update humidity value
    err = sensor_channel_get(bme_280_device, SENSOR_CHAN_HUMIDITY, &temp_hum_measurements.humidity);
    if (err != 0)
    {
        LOG_INF("Error in retreiving sample %d", err);
    }
    // Disable spi to save power
    pm_device_action_run(bme_spi_device, PM_DEVICE_ACTION_SUSPEND);
}

// --- getters -----------------------------------------------------------------
// Getter for temperature value
struct sensor_value get_temperature_measurement(void)
{
    return temp_hum_measurements.temperature;
}

// Getter for humidity value
struct sensor_value get_humidity_measurement(void)
{
    return temp_hum_measurements.humidity;
}