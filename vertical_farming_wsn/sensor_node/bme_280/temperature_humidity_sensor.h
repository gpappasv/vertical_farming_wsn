#ifndef TEMPERATURE_HUMIDITY_SENSOR_H
#define TEMPERATURE_HUMIDITY_SENSOR_H

// --- includes ----------------------------------------------------------------
#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>

// --- structs -----------------------------------------------------------------
typedef struct bme_measurements_s
{
    // Temp 21.62 oC is represented as val1 = 21, val2 = 620000
    struct sensor_value temperature;
    // Humidity 44.62% is represented as val1 = 44, val2 = 620000
    struct sensor_value humidity;
} bme_measurements_t;

// --- functions declarations --------------------------------------------------
void measure_temperature(void);
void measure_humidity(void);
bool init_bme280(void);
struct sensor_value get_temperature_measurement(void);
struct sensor_value get_humidity_measurement(void);

#endif // TEMPERATURE_HUMIDITY_SENSOR_H