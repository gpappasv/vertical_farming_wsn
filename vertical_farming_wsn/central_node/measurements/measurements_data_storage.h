#ifndef MEASUREMENTS_DATA_STORAGE_H
#define MEASUREMENTS_DATA_STORAGE_H

// --- includes ----------------------------------------------------------------
#include <stdbool.h>
#include <stdint.h>
#include "../../common/common.h"

// --- functions declartations -------------------------------------------------
void set_temperature_measurement_value(struct bt_conn *conn, int32_t measured_temperature);
void set_humidity_measurement_value(struct bt_conn *conn, int32_t measured_humidity);
void set_soil_moisture_measurement_value(struct bt_conn *conn, int32_t soil_moisture);
void set_light_intensity_measurement_value(struct bt_conn *conn, int32_t light_intensity);

void set_configuration_id_value(struct bt_conn *conn, uint8_t configuration_id);
void set_battery_level_value(struct bt_conn *conn, uint8_t battery_level);

bool measurements_and_device_data(void);

void clear_measurement_data(void);
void get_all_ble_connection_handles(void);

void print_all_measurements_and_connection_handles(void);

measurements_data_t* get_measurements_data(void);
row_mean_data_t* get_row_mean_data(void);

#endif // MEASUREMENTS_DATA_STORAGE_H