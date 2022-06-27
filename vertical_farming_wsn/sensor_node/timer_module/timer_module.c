// --- includes ----------------------------------------------------------------
#include <logging/log.h>
#include "timer_module.h"
#include "../light_sensor/light_sensor.h"
#include "../adc_calibration/adc_calibration.h"

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(timer_m);

// --- structs -----------------------------------------------------------------
static struct k_timer adc_calibration_timer;

// --- interrupt handlers  -------------------------------------------

static void adc_calibration_timer_handler(struct k_timer *timer_id)
{
    struct k_work *adc_calibration_item = get_adc_calibrate_work_item();
    k_work_submit(adc_calibration_item);
}

// --- functions declarations -------------------------------------------
void init_adc_calibration_timer(void)
{
    k_timer_init(&adc_calibration_timer, adc_calibration_timer_handler, NULL);
}

void start_adc_calibration_timer(k_timeout_t duration, k_timeout_t period)
{
    k_timer_start(&adc_calibration_timer, duration, period);
}

void stop_adc_calibration_timer(k_timeout_t duration, k_timeout_t period)
{
    k_timer_stop(&adc_calibration_timer);
}