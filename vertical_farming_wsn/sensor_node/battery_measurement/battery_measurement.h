#ifndef INCLUDE_BATTERY_MEASUREMENT_H
#define INCLUDE_BATTERY_MEASUREMENT_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// --- functions declarations --------------------------------------------------
int16_t get_battery_level(void);
int16_t adc_measure_battery_level(void);
int16_t init_battery_adc_channel(void);

#endif /* INCLUDE_BATTERY_MEASUREMENT_H */