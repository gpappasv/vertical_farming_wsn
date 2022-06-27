#ifndef ENVIRONMENT_CONTROL_CONFIG_H
#define ENVIRONMENT_CONTROL_CONFIG_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// --- defines -----------------------------------------------------------------
#define DEFAULT_TEMPERATURE_THRESHOLD 2800 // 2842 = 28.42 oC
#define DEFAULT_HUMIDITY_THRESHOLD    7000 // 7212 = 72.12%
#define DEFAULT_SOIL_MOISTURE_THRESHOLD 6000 // 6213 = 62.13%
#define DEFAULT_LIGHT_EXPOSURE_THRESHOLD 100 // TODO: Random number, TBD after light sensor is functional
#define ROW_1_CONTROL_CONFIGURATION_FLASH_KEY 0
#define ROW_2_CONTROL_CONFIGURATION_FLASH_KEY 1
#define ROW_3_CONTROL_CONFIGURATION_FLASH_KEY 2
#define ROW_4_CONTROL_CONFIGURATION_FLASH_KEY 3
#define ROW_5_CONTROL_CONFIGURATION_FLASH_KEY 4

// --- structs -----------------------------------------------------------------
// this struct is about user row configuration
// it contains info about the on off switch of fan/water/light
// about the automatic/manual control of fan/water/light
// and the thresholds for hum/temp/soil/light on which fan/water/light will turn
// on  in case of automatic control
typedef struct row_control_s
{
    // switches
    bool fan_switch;
    bool water_switch;
    bool light_switch;
    // flag that indicates if row is manually or automatically controled
    bool automatic_control;
    // thresholds for automatic control
    int32_t temp_threshold;
    int32_t humidity_threshold;
    int32_t light_threshold;
    int32_t soil_moisture_threshold;
}row_control_t;

typedef struct row_status_s
{
    // store the mean measurements for the row
    int32_t row_temp;
    int32_t row_hum;
    int32_t row_soil_moisture;
    int32_t row_light_exposure;
    uint8_t row_id;
    // this field indicates if the row has at least one sensor node
    bool    row_registered;
    // user sets those parameters
    row_control_t row_control;
}row_status_t;

// --- functions declarations --------------------------------------------------
void initialize_row_control_configuration(void);

// --- getters / setters for mean row measurements
void set_row_current_temperature(int32_t value, uint8_t index);
int32_t get_row_current_temperature(uint8_t index);

void set_row_current_humidity(int32_t value, uint8_t index);
int32_t get_row_current_humidity(uint8_t index);

void set_row_current_soil_moisture(int32_t value, uint8_t index);
int32_t get_row_current_soil_moisture(uint8_t index);

void set_row_current_light_exposure(int32_t value, uint8_t index);
int32_t get_row_current_light_exposure(uint8_t index);

void set_row_registered(uint8_t index);
bool get_row_registered(uint8_t index);
void reset_row_status(uint8_t index);

// --- getters / setters for user config parameters
void set_row_temp_threshold(int32_t value, uint8_t index);
int32_t get_row_temp_threshold(uint8_t index);

void set_row_hum_threshold(int32_t value, uint8_t index);
int32_t get_row_hum_threshold(uint8_t index);

void set_row_light_threshold(int32_t value, uint8_t index);
int32_t get_row_light_threshold(uint8_t index);

void set_row_soil_moisture_threshold(int32_t value, uint8_t index);
int32_t get_row_soil_moisture_threshold(uint8_t index);

void set_row_water_switch(bool value, uint8_t index);
bool get_row_water_switch(uint8_t index);

void set_row_fan_switch(bool value, uint8_t index);
bool get_row_fan_switch(uint8_t index);

void set_row_light_switch(bool value, uint8_t index);
bool get_row_light_switch(uint8_t index);

void set_row_automatic_control(bool value, uint8_t index);
bool get_row_automatic_control(uint8_t index);

void update_row_control_config_params_in_nvs(uint8_t index);

#endif // ENVIRONMENT_CONTROL_CONFIG_H