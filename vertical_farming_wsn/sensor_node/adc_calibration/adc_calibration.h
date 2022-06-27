#ifndef INCLUDE_ADC_CALIBRATION_H
#define INCLUDE_ADC_CALIBRATION_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>

// --- functions declarations --------------------------------------------------
int16_t init_adc_calibration_channel(void);
struct k_work *get_adc_calibrate_work_item(void);

#endif /* INCLUDE_ADC_CALIBRATION_H */