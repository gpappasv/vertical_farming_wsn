// --- includes ----------------------------------------------------------------
#include <drivers/adc.h>
#include <logging/log.h>
#include <zephyr.h>

#define ADC_RESOLUTION_CALIBRATION_BITS 12
#define ADC_CALIBRATION_CHANNEL 3
#define ADC_CALIBRATION_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20)

// --- logging -----------------------------------------------------------------
LOG_MODULE_DECLARE(adc_interface_m);

// --- static variables definitions --------------------------------------------
static int16_t calibration_sample;

// --- static functions declarations -------------------------------------------
/**
 * @brief
 *
 * This callback is called after each ADC sampling of the calibration channel
 * is finished. This particular callback adds no functionality other than
 * logging info that ADC has finished calibration.
 *
 * @param dev
 * @param sequence
 * @param sampling_index
 * @return enum adc_action
 */
static enum adc_action
adc_calibration_sampling_done_callback(const struct device *dev, const struct adc_sequence *sequence, uint16_t sampling_index)
{
    LOG_INF("ADC Calibration Done!");
    return ADC_ACTION_FINISH;
}

// --- structs -----------------------------------------------------------------
static const struct device *adc_dev = DEVICE_DT_GET(DT_ALIAS(adcctrl));

static struct k_work adc_calibrate_work_item;

/* ADC Channel Configuration */
static const struct adc_channel_cfg adc_calibration_channel =
{
    .gain = ADC_GAIN_1_5,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_CALIBRATION_ACQUISITION_TIME,
    .channel_id = ADC_CALIBRATION_CHANNEL,
    .differential = 0,
    .input_positive = 1 + ADC_CALIBRATION_CHANNEL,
};

/*  ADC sequence options.
    The ADC finished sampling callback is defined here */
static const struct adc_sequence_options adc_calibration_sequence_options =
{
    .interval_us = 0,
    .callback = adc_calibration_sampling_done_callback,
    .extra_samplings = 0
};

/*  ADC read sequence for calibration channel. For calibration
    a currently ADC unused channel is used. .calibration option
    has to be set to true, in order to trigger the offset
    calibration task of the SAADC */

static const struct adc_sequence calibration_sequence =
{
    .options = &adc_calibration_sequence_options,
    .channels = BIT(ADC_CALIBRATION_CHANNEL),
    .buffer = &calibration_sample,
    .buffer_size = sizeof(calibration_sample),
    .resolution = ADC_RESOLUTION_CALIBRATION_BITS,
    .oversampling = 8,
    .calibrate = true
};

// --- static functions declarations -------------------------------------------
static void adc_calibrate_work_handler(struct k_work *work);

// --- static functions definitions --------------------------------------------
/**
 * @brief 
 * Workqueue handler for ADC Calibration
 * 
 */
static void adc_calibrate_work_handler(struct k_work *work)
{
    adc_read(adc_dev, &calibration_sequence);
}

// --- functions declarations --------------------------------------------------
/**
 * @brief ADC Calibration Channel Initialization
 * 
 * @return int16_t error
 * 
 */
int16_t init_adc_calibration_channel(void)
{
    int err;
    err = adc_channel_setup(adc_dev, &adc_calibration_channel);
    if (err)
    {
        LOG_INF("Error setting up calibration channel.");
    }

    k_work_init(&adc_calibrate_work_item, adc_calibrate_work_handler);

    return err;
}

/**
 * @brief 
 *  Get the adc calibrate work item object.
 *  Used by the timer module IRQ handler to
 *  put work item in system workqueue.       
 * 
 * @return struct k_work* 
 */
struct k_work *get_adc_calibrate_work_item(void)
{
    return &adc_calibrate_work_item;
}