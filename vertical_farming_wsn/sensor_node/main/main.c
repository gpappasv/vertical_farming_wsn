// --- includes ----------------------------------------------------------------
#include <zephyr.h>

#include <logging/log.h>
#include <drivers/gpio.h>
#include <zephyr.h>
#include "../bme_280/temperature_humidity_sensor.h"
#include "../ble_server/ble_measurement_service.h"
#include "../ble_server/ble_conn_control.h"
#include "../soil_moisture/soil_moisture.h"
#include "../battery_measurement/battery_measurement.h"
#include "../light_sensor/light_sensor.h"
#include "../adc_calibration/adc_calibration.h"
#include "../timer_module/timer_module.h"
#include "../watchdog_timer/watchdog_timer.h"
#include "../flash_system/flash_system.h"
#include <pm/device.h>
#include <smf.h>
#include <toolchain.h>

#include <mgmt/mcumgr/smp_bt.h>
#include "os_mgmt/os_mgmt.h"
#include "img_mgmt/img_mgmt.h"

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(main_m);

// --- defines -----------------------------------------------------------------
// size of stack area used by each thread
#define STACKSIZE 1024
// scheduling priority used by each thread
// TODO: probably add a central headerfile with priorities
#define PRIORITY 7
#define BUTTON0 DT_ALIAS(button0)
#define SYS_REBOOT_WARM 0

// --- enums -------------------------------------------------------------------
// List of states
enum ble_state_e
{
    CONFIGURE_STATE,
    OPERATING_STATE
};

// --- static variables definitions --------------------------------------------
// Forward declaration of state table
// TODO: maybe add state to measure and notify battery level when a timer interrupt occures?
static const struct smf_state fsm_states[];
static struct gpio_dt_spec clean_row_id_button = GPIO_DT_SPEC_GET(BUTTON0, gpios);
// --- button 0 callback
struct gpio_callback clear_id_button_cb;
static struct k_work work_clear_id;

// --- variables definitions ---------------------------------------------------
// User defined object
struct user_object_s
{
    // This must be first
    struct smf_ctx ctx;

    // Other state specific data add here
} user_object;

struct k_sem configure_done_sem;

// --- static function declarations --------------------------------------------
static void clear_id_cb(const struct device *port,
                        struct gpio_callback *cb,
                        gpio_port_pins_t pins);
static void clear_id_work_function(struct k_work *item);
static void setup_clear_id_button(void);
static void configure_state_entry(void *o);
static void configure_state_run(void *o);

static void operating_state_run(void *o);

static void sensor_node_fsm(void);

// --- static function definitions ---------------------------------------------
K_THREAD_DEFINE(sensor_fsm_id, STACKSIZE, sensor_node_fsm, NULL, NULL, NULL,
                PRIORITY, 0, 0);
/**
 * @brief function that is called after pressing button 0, adds clear_id_work_function()
 *        to the workqueue. clear_id_work_function clears configuration id and
 *        resets the sensor node
 *
 */
static void clear_id_cb(const struct device *port,
                        struct gpio_callback *cb,
                        gpio_port_pins_t pins)
{
    k_work_submit(&work_clear_id);
}

/**
 * @brief function that is called after pressing button 0, clears configuration id
 *        and resets the node
 *
 */
static void clear_id_work_function(struct k_work *item)
{
    int ret = 0;
    ret = nvs_delete(get_file_system_handle(), DEVICE_CONFIGURATION_FLASH_KEY);
    LOG_INF("Delete configuration id %d", ret);
    sys_reboot(SYS_REBOOT_WARM);
}

/**
 * @brief Set up the clear row id button. After setting it up (button0),
 *        pressing it will result in clearing the row id and resetting the node
 *
 */
static void setup_clear_id_button(void)
{
    // Configure clean row id gpio button
    if (!device_is_ready(clean_row_id_button.port))
    {
        LOG_INF("Error: button GPIO device is not ready");
    }
    gpio_pin_configure_dt(&clean_row_id_button, GPIO_INPUT);
    gpio_init_callback(&clear_id_button_cb, clear_id_cb, BIT(clean_row_id_button.pin));
    gpio_add_callback(clean_row_id_button.port, &clear_id_button_cb);
    gpio_pin_interrupt_configure_dt(&clean_row_id_button, GPIO_INT_EDGE_TO_ACTIVE);
    k_work_init(&work_clear_id, clear_id_work_function);
}
// --- CONFIGURE STATE
static void configure_state_entry(void *o)
{
    os_mgmt_register_group();
    img_mgmt_register_group();
    smp_bt_register();

    const static struct device *bme_spi_device = DEVICE_DT_GET_ANY(nordic_nrf_spim);

    k_sem_init(&configure_done_sem, 0, 1);
    // --- system init ---
    init_watchdog();
    // Start bluetooth
    ble_init();
    setup_clear_id_button();
    // Flash system init
    flash_system_init();
    if (!init_bme280())
    {
        // If bme280 is not ready, perform software reset
        sys_reboot(SYS_REBOOT_WARM);
    }
    // adc init
    init_battery_adc_channel();
    init_light_adc_channel();
    init_soil_adc_channel();
    init_adc_calibration_channel();

    init_adc_calibration_timer();
    start_adc_calibration_timer(K_SECONDS(1), K_SECONDS(30));

    pm_device_action_run(bme_spi_device, PM_DEVICE_ACTION_SUSPEND);
}
static void configure_state_run(void *o)
{
    uint8_t dev_id_buffer;

    // Check if device is configured, if not, enter the if statement and wait for configuration
    if (nvs_read(get_file_system_handle(), DEVICE_CONFIGURATION_FLASH_KEY, &dev_id_buffer, sizeof(dev_id_buffer)) != sizeof(dev_id_buffer))
    {
        // Advertise configure data after init
        start_configure_state_adv();
        k_sem_take(&configure_done_sem, K_FOREVER);
    }
    else
    {
        LOG_INF("Configuration id is: %d", dev_id_buffer);
    }
    smf_set_state(SMF_CTX(&user_object), &fsm_states[OPERATING_STATE]);
}

static void operating_state_run(void *o)
{
    // Start advertising operating adv data
    start_operating_state_adv();

    k_thread_abort(sensor_fsm_id);
}

// Populate state table
static const struct smf_state fsm_states[] =
    {
        [CONFIGURE_STATE] = SMF_CREATE_STATE(configure_state_entry, configure_state_run, NULL),
        [OPERATING_STATE] = SMF_CREATE_STATE(NULL, operating_state_run, NULL),
};

static void sensor_node_fsm(void)
{
    int32_t ret;
    // Set initial state
    smf_set_initial(SMF_CTX(&user_object), &fsm_states[CONFIGURE_STATE]);

    // Run the state machine
    while (1)
    {
        // State machine terminates if a non-zero value is returned
        ret = smf_run_state(SMF_CTX(&user_object));
        if (ret)
        {
            // handle return code and terminate state machine
            break;
        }
    }
}
