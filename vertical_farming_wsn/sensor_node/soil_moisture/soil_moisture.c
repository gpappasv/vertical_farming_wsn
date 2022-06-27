// --- includes ----------------------------------------------------------------
#include <drivers/adc.h>
#include <logging/log.h>
#include <zephyr.h>

#define ADC_RESOLUTION_SOIL_BITS 12
#define ADC_SOIL_MOISTURE_CHANNEL 1 /* P0.02 */
#define ADC_SOIL_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20)
#define ADC_SOIL_MOISTURE_SCALING 10000
/* Measured value in air was 3485 but we have to account for humidity of air during measurement */
#define MAX_SOIL_MOISTURE_VAL 3600
/* Measured value fully submerged in water was 1700 but we lower the boundary a little bit just in case */
#define MIN_SOIL_MOISTURE_VAL 1600
// --- logging -----------------------------------------------------------------
LOG_MODULE_DECLARE(adc_interface_m);

// --- static variables definitions --------------------------------------------
static int16_t soil_moisture_sample;

// --- static functions declarations -------------------------------------------
/**
 * @brief
 *
 * ADC soil humidity level sampling done callback.
 * Functionality serves to zero out negative ADC results resulting
 * from ground offset error.
 * @param dev
 * @param sequence
 * @param sampling_index
 * @return enum adc_action
 */
static enum adc_action
adc_soil_sampling_done_callback(const struct device *dev, const struct adc_sequence *sequence, uint16_t sampling_index)
{

    // Zero out negative results due to ground bounce.
    soil_moisture_sample = (soil_moisture_sample > 0) ? soil_moisture_sample : 0;

    return ADC_ACTION_FINISH;
}

// --- structs -----------------------------------------------------------------
static const struct device *adc_dev = DEVICE_DT_GET(DT_ALIAS(adcctrl));

/* ADC Battery Channel Configuration */
static const struct adc_channel_cfg soil_adc_channel =
    {
        .gain = ADC_GAIN_1_5,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_SOIL_ACQUISITION_TIME,
        .channel_id = ADC_SOIL_MOISTURE_CHANNEL,
        .differential = 0,
        .input_positive = 1 + ADC_SOIL_MOISTURE_CHANNEL,
};

/*  ADC sequence options.
    The ADC finished sampling callback is defined here */

static const struct adc_sequence_options adc_soil_sequence_options =
    {
        .interval_us = 0,
        .callback = adc_soil_sampling_done_callback,
        .extra_samplings = 0};

/*  ADC read sequence for soil humidity measurement channel. */
static const struct adc_sequence soil_sequence =
    {
        .options = &adc_soil_sequence_options,
        .channels = BIT(ADC_SOIL_MOISTURE_CHANNEL),
        .buffer = &soil_moisture_sample,
        .buffer_size = sizeof(soil_moisture_sample),
        .resolution = ADC_RESOLUTION_SOIL_BITS,
        .oversampling = 8,
        .calibrate = 0};

// --- functions declarations --------------------------------------------------
/**
 * @brief
 * Initialize soil humidity ADC Channel
 *
 * @return int16_t
 */
int16_t
init_soil_adc_channel(void)
{
    int err;
    err = adc_channel_setup(adc_dev, &soil_adc_channel);
    if (err)
    {
        LOG_INF("Error setting up soil measurement channel.");
    }

    return err;
}

/**
 * @brief
 * Triggers an ADC read sequence for soil humidity measurement
 *
 * @return int16_t
 */
int16_t adc_measure_soil_moisture()
{
    int err;
    err = adc_read(adc_dev, &soil_sequence);
    return err;
}

/**
 * @brief
 * Get soil humidity level as a percentage 0..100
 *  but with a scaling of 100
 *  
 *  10000 = 100% Physical value
 * 
 *  Perfect Accuracy:
 *  Suppose 3000 as raw ADC value
 *  After offset correction 2133 - 1600 = 533
 *  result = 533 / (3600 - 1600) = 0.2665
 *  result = 1 - 0.2665 = 73.35%
 * 
 *  Achieved Accuracy:
 *  result = 533 * 10000 / (3600 - 1600) = 2665
 *  result = 10000 - 2665 = 7335 = 73.35%
 * 
 *  No loss up until second decimal place which is more than enough.
 *
 * @return int16_t
 */
int16_t adc_get_soil_moisture(void)
{
    int16_t soil_moisture_offset_corrected;
    int16_t result = 0;
    if (soil_moisture_sample <= MAX_SOIL_MOISTURE_VAL)
    {
        soil_moisture_offset_corrected = ((soil_moisture_sample - MIN_SOIL_MOISTURE_VAL) >= 0) ? (soil_moisture_sample - MIN_SOIL_MOISTURE_VAL) : 0;
        result = (soil_moisture_offset_corrected * ADC_SOIL_MOISTURE_SCALING) / (MAX_SOIL_MOISTURE_VAL - MIN_SOIL_MOISTURE_VAL);
        result = (1 * ADC_SOIL_MOISTURE_SCALING) - result;
    }

    return result;
}
