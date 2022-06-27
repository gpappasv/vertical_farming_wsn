// --- includes ----------------------------------------------------------------
#include <drivers/adc.h>
#include <logging/log.h>
#include <zephyr.h>

// --- defines -----------------------------------------------------------------
#define ADC_RESOLUTION_LIGHT_BITS 12
#define ADC_LIGHT_INTENSITY_CHANNEL 0 /* P0.03 */
#define ADC_LIGHT_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20)

// --- logging -----------------------------------------------------------------
LOG_MODULE_REGISTER(adc_interface_m);

// --- static variables definitions --------------------------------------------
static int16_t light_intensity_sample;

// --- static functions declarations -------------------------------------------
/**
 * @brief
 *
 * This callback is called after each ADC sampling of the calibration channel
 * is finished. Functionality serves to zero out possible negative
 * ADC samples due to ground bounce.
 * @param dev
 * @param sequence
 * @param sampling_index
 * @return enum adc_action
 */
static enum adc_action
adc_light_sampling_done_callback(const struct device *dev, const struct adc_sequence *sequence, uint16_t sampling_index)
{
    // Zero out negative results due to ground bounce.
    light_intensity_sample = (light_intensity_sample > 0) ? light_intensity_sample : 0;

    return ADC_ACTION_FINISH;
}

// --- structs -----------------------------------------------------------------
static const struct device *adc_dev = DEVICE_DT_GET(DT_ALIAS(adcctrl));

/* ADC Light Sensor Channel Configuration */
static const struct adc_channel_cfg light_adc_channel =
{
    .gain = ADC_GAIN_1_5,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_LIGHT_ACQUISITION_TIME,
    .channel_id = ADC_LIGHT_INTENSITY_CHANNEL,
    .differential = 0,
    .input_positive = 1 + ADC_LIGHT_INTENSITY_CHANNEL,
};

/*  ADC sequence options.
    The ADC finished sampling callback is defined here */

static const struct adc_sequence_options adc_light_sequence_options =
{
    .interval_us = 0,
    .callback = adc_light_sampling_done_callback,
    .extra_samplings = 0
};

/*  ADC read sequence for light measurement channel. */
static const struct adc_sequence light_sequence =
{
    .options = &adc_light_sequence_options,
    .channels = BIT(ADC_LIGHT_INTENSITY_CHANNEL),
    .buffer = &light_intensity_sample,
    .buffer_size = sizeof(light_intensity_sample),
    .resolution = ADC_RESOLUTION_LIGHT_BITS,
    .oversampling = 3,
    .calibrate = 0
};

// --- functions declarations -------------------------------------------
/**
 * @brief
 * Triggers and ADC read for light intensity measurement
 *
 * @return int16_t
 */
int16_t measure_light_intensity(void)
{
    int err;
    err = adc_read(adc_dev, &light_sequence);
    return err;
}

/**
 * @brief
 * Initialize light intensity measurement channel
 *
 * @return int16_t
 */
void init_light_adc_channel(void)
{
    int err;
    err = adc_channel_setup(adc_dev, &light_adc_channel);
    if (err)
    {
        LOG_INF("Error setting up light measurement channel.");
    }
}

/**
 * @brief Get the light intensity ADC value
 *
 * @return int16_t
 */
int16_t get_light_intensity(void)
{
    return light_intensity_sample;
}
