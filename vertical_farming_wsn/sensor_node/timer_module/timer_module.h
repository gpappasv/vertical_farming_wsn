#ifndef INCLUDE_TIMER_MODULE_H
#define INCLUDE_TIMER_MODULE_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <zephyr.h>

// --- functions declarations -------------------------------------------------
void init_adc_calibration_timer(void);
void start_adc_calibration_timer(k_timeout_t duration, k_timeout_t period);
void stop_adc_calibration_timer(k_timeout_t duration, k_timeout_t period);

#endif /* INCLUDE_TIMER_MODULE_H */