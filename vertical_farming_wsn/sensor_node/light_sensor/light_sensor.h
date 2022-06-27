#ifndef INCLUDE_LIGHT_SENSOR_H
#define INCLUDE_LIGHT_SENSOR_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// --- functions declarations --------------------------------------------------
int16_t measure_light_intensity(void);
int16_t get_light_intensity(void);
void init_light_adc_channel(void);

#endif /* INCLUDE_LIGHT_SENSOR_H */