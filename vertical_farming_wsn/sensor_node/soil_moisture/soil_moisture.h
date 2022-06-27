#ifndef INCLUDE_SOIL_MOISTURE_H
#define INCLUDE_SOIL_MOISTURE_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>

// --- functions declarations --------------------------------------------------
int16_t adc_get_soil_moisture(void);
int16_t adc_measure_soil_moisture(void);
int16_t init_soil_adc_channel(void);

#endif /* INCLUDE_SOIL_MOISTURE_H */