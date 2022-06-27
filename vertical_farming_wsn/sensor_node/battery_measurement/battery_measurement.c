// --- includes ----------------------------------------------------------------
#include <drivers/adc.h>
#include <logging/log.h>
#include <zephyr.h>

#define ADC_RESOLUTION_BATTERY_BITS 12
#define ADC_BATTERY_MEASUREMENT_CHANNEL 2 /* P0.04 */
#define ADC_BATTERY_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20)

// --- logging -----------------------------------------------------------------
LOG_MODULE_DECLARE(adc_interface_m);

/* A look-up table with capacity - voltage data will be added here
   to extract the percentage of remaining capacity.

   But first we must somehow map 4.23 V to either 0.6 1.2 1.8 2.4 3 or 3.6V through
   appropriate resistors.  */

// --- static variables definitions --------------------------------------------
static int16_t battery_level_sample;

// --- static functions declarations -------------------------------------------

/**
 * @brief 
 * 
 * ADC battery level sampling done callback.
 * Functionality serves to zero out negative ADC results resulting
 * from ground offset error.
 * @param dev 
 * @param sequence 
 * @param sampling_index 
 * @return enum adc_action 
 */
static enum adc_action
adc_battery_sampling_done_callback(const struct device *dev, const struct adc_sequence *sequence, uint16_t sampling_index)
{
    // Zero out negative results due to ground bounce.
    battery_level_sample = (battery_level_sample > 0) ? battery_level_sample : 0;

    return ADC_ACTION_FINISH;
}

// --- structs -----------------------------------------------------------------
static const struct device *adc_dev = DEVICE_DT_GET(DT_ALIAS(adcctrl));

/* ADC Battery Channel Configuration */
static const struct adc_channel_cfg battery_adc_channel =
{
    .gain = ADC_GAIN_1_5,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_BATTERY_ACQUISITION_TIME,
    .channel_id = ADC_BATTERY_MEASUREMENT_CHANNEL,
    .differential = 0,
    .input_positive = 1 + ADC_BATTERY_MEASUREMENT_CHANNEL,
};

/*  ADC sequence options.
    The ADC finished sampling callback is defined here */

static const struct adc_sequence_options adc_battery_sequence_options =
{
    .interval_us = 0,
    .callback = adc_battery_sampling_done_callback,
    .extra_samplings = 0
};

/*  ADC read sequence for battery measurement channel. */
static const struct adc_sequence battery_sequence =
{
    .options = &adc_battery_sequence_options,
    .channels = BIT(ADC_BATTERY_MEASUREMENT_CHANNEL),
    .buffer = &battery_level_sample,
    .buffer_size = sizeof(battery_level_sample),
    .resolution = ADC_RESOLUTION_BATTERY_BITS,
    .oversampling = 2,
    .calibrate = 0
};

// --- functions declarations --------------------------------------------------

/**
 * @brief 
 * Triggers an ADC read sequence for battery measurement.
 * 
 * 
 * @return int16_error 
 */
int16_t measure_battery_level(void)
{
    int err;
    err = adc_read(adc_dev, &battery_sequence);
    return err;
}

/**
 * @brief 
 * Initialize the battery measurement ADC Channel
 * 
 * @return int16_t 
 */
int16_t init_battery_adc_channel(void)
{
    int err;
    err = adc_channel_setup(adc_dev, &battery_adc_channel);
    if (err)
    {
        LOG_INF("Error setting up battery measurement channel.");
    }

    return err;
}

/**
 * @brief 
 * Will provide actual battery level percentage based
 * on look up table.
 * 
 * @return int16_t 
 */
int16_t adc_get_battery_level(void)
{
    // TODO: Figure out LUT for percentage
    return battery_level_sample;
}
