/**
 * Description:
 *
 * FSM to take measurements from connected nodes
 *
 */

// --- includes ----------------------------------------------------------------
#include "measurements_fsm.h"
#include "measurements_data_storage.h"
#include "../ble_client/ble_connection_data.h"
#include "../../common/common.h"
#include "../internal_uart/internal_uart.h"
#include "../../common/com_protocol/com_protocol.h"
#include "../environment_control/environment_control_config.h"
#include "../environment_control/environment_control_fsm.h"
#include <zephyr.h>

#include <smf.h>
#include <logging/log.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(measurements_m);

// --- enums -------------------------------------------------------------------
// List of states
enum ble_state_e
{
    TAKE_MEASUREMENTS,
    CALCULATE_MEAN_MEASUREMENTS,
    ENVIRONMENT_CONTROL,
    SEND_DATA_TO_9160,
    THREAD_SLEEP
};

// --- structs -----------------------------------------------------------------
// User defined object
struct user_object_s
{
    // This must be first
    struct smf_ctx ctx;

    // a copy of measurements_data_t measurement_data will be placed here
    measurements_data_t *measurements_data;
    // will get the memory location of row_mean_data_t mean_row_measurements
    row_mean_data_t *row_mean_data;
} measurements_fsm_user_object;

// --- static function declarations --------------------------------------------
static void take_measurements_entry(void *o);
static void take_measurements_run(void *o);

static void calculate_mean_measurements_entry(void *o);
static void calculate_mean_measurements_run(void *o);

static void environment_control_run(void *o);

static void send_data_to_9160_run(void *o);

static void thread_sleep_run(void *o);

// --- static variables definitions --------------------------------------------
// Populate state table
static const struct smf_state measurement_states[] = {
    [TAKE_MEASUREMENTS] = SMF_CREATE_STATE(take_measurements_entry, take_measurements_run, NULL),
    [CALCULATE_MEAN_MEASUREMENTS] = SMF_CREATE_STATE(calculate_mean_measurements_entry, calculate_mean_measurements_run, NULL),
    [ENVIRONMENT_CONTROL] = SMF_CREATE_STATE(NULL, environment_control_run, NULL),
    [SEND_DATA_TO_9160] = SMF_CREATE_STATE(NULL, send_data_to_9160_run, NULL),
    [THREAD_SLEEP] = SMF_CREATE_STATE(NULL, thread_sleep_run, NULL),
};

// --- variables definitions ---------------------------------------------------
// Semaphore to know when a characteristic read process is completed
struct k_sem read_response_sem;
// This semaphore is meant to be increased and decreased only by ble connected and disconnected cb functions
struct k_sem at_least_one_active_connection_sem;
extern struct k_event env_control_event;

// --- static function definitions ---------------------------------------------
// --- TAKE_MEASUREMENTS state ---
static void take_measurements_entry(void *o)
{
    // First clean measurement data as well as row mean measurement values
    clear_measurement_data();
    // Then get the conn handles from all connected devices
    get_all_ble_connection_handles();
}

static void take_measurements_run(void *o)
{
    bool measurements_taken = false;
    // Take measurements and device data
    measurements_taken = measurements_and_device_data();

    if (measurements_taken)
    {
        // Just for debug -> will be deleted
        //print_all_measurements_and_connection_handles();
        smf_set_state(SMF_CTX(&measurements_fsm_user_object), &measurement_states[CALCULATE_MEAN_MEASUREMENTS]);
    }
    else
    {
        LOG_INF("No measurements taken");
        // if measurements_taken is false, then we have no connected devices -> thread sleep
        smf_set_state(SMF_CTX(&measurements_fsm_user_object), &measurement_states[THREAD_SLEEP]);
    }
}

// --- CALCULATE_MEAN_MEASUREMENTS state ---
static void calculate_mean_measurements_entry(void *o)
{
    struct user_object_s *user_ctx = (struct user_object_s *)o;

    // User measurement data and row mean data getters
    user_ctx->measurements_data = get_measurements_data();
    user_ctx->row_mean_data = get_row_mean_data();
}

static void calculate_mean_measurements_run(void *o)
{
    struct user_object_s *user_ctx = (struct user_object_s *)o;

    // Go through all rows
    for (int row_index = 0; row_index < MAX_CONFIGURATION_ID; row_index++)
    {
        // Counter to know how many sensor nodes exist on a specific row
        // will be used as: mean_humidity = (humidity_node1 + humidity_node2 +...) / measurements_counter
        uint8_t measurements_counter = 0;

        // Temporary variables to calculate mean measurement values
        int32_t mean_ambient_humidity = 0;
        int32_t mean_ambient_temperature = 0;
        int32_t mean_soil_moisture = 0;
        int32_t mean_light_intensity = 0;

        // Check if row is registered/active (if at least one sensor node exist on this row), if not, skip
        if (user_ctx->row_mean_data[row_index].is_row_registered)
        {
            // If row is registered/active, grab all measurements from the nodes that belong to this particular row
            // We are going through all connected sensor nodes and check on which row they belong
            for (uint8_t measurement_data_index = 0; measurement_data_index < BLE_MAX_CONNECTIONS; measurement_data_index++)
            {
                // Check if a sensor node belonds to the desired row
                if (user_ctx->measurements_data[measurement_data_index].row_id == user_ctx->row_mean_data[row_index].row_id)
                {
                    // Mean row humidity
                    mean_ambient_humidity += user_ctx->measurements_data[measurement_data_index].ambient_hum_measurement;
                    // Mean row temp
                    mean_ambient_temperature += user_ctx->measurements_data[measurement_data_index].ambient_temp_measurement;
                    // Mean light intensity
                    mean_light_intensity += user_ctx->measurements_data[measurement_data_index].light_measurement;
                    // Mean soil moisture
                    mean_soil_moisture += user_ctx->measurements_data[measurement_data_index].soil_moisture_measurement;
                    // Increment measurement counter every time we find a node that exists on the row that is being currently referenced (row_index)
                    measurements_counter++;
                }
            }
            // Calculate means (devide the measurement sum by the sensor nodes number)
            mean_ambient_humidity /= measurements_counter;
            mean_ambient_temperature /= measurements_counter;
            mean_light_intensity /= measurements_counter;
            mean_soil_moisture /= measurements_counter;
            measurements_counter = 0;
            // Save mean values for the corresponding row
            user_ctx->row_mean_data[row_index].mean_row_humidity = mean_ambient_humidity;
            user_ctx->row_mean_data[row_index].mean_row_light = mean_light_intensity;
            user_ctx->row_mean_data[row_index].mean_row_soil_moisture = mean_soil_moisture;
            user_ctx->row_mean_data[row_index].mean_row_temp = mean_ambient_temperature;
            // Update the status of fan/water/lights for the corresponding row
            user_ctx->row_mean_data[row_index].is_fan_active = get_row_fan_switch(row_index);
            user_ctx->row_mean_data[row_index].is_watering_active = get_row_water_switch(row_index);
            user_ctx->row_mean_data[row_index].are_lights_active = get_row_light_switch(row_index);
        }
    }

    // TODO: just for debug
    for (uint8_t index = 0; index < MAX_CONFIGURATION_ID; index++)
    {
        if (user_ctx->row_mean_data[index].is_row_registered)
        {
            LOG_INF(" ------- MEAN DATA --------- row id: %d", user_ctx->row_mean_data[index].row_id);
            LOG_INF("Mean Temperature is: %d.%d C", user_ctx->row_mean_data[index].mean_row_temp / 100, user_ctx->row_mean_data[index].mean_row_temp % 100);
            LOG_INF("Mean Humidity is: %d.%d percent", user_ctx->row_mean_data[index].mean_row_humidity / 100, user_ctx->row_mean_data[index].mean_row_humidity % 100);
            LOG_INF("Mean Soil moisture is: %d percent", user_ctx->row_mean_data[index].mean_row_soil_moisture);
            LOG_INF("Mean Light intensity is: %d", user_ctx->row_mean_data[index].mean_row_light);
        }
    }

    smf_set_state(SMF_CTX(&measurements_fsm_user_object), &measurement_states[ENVIRONMENT_CONTROL]);
}

// --- ENVIRONMENT_CONTROL state ---
static void environment_control_run(void *o)
{
    struct user_object_s *user_ctx = (struct user_object_s *)o;

    // Store the mean row measurements on the environment control module
    // environment control module will check what we set here to control the airflow/water/lights
    for(uint8_t row_id = 0; row_id < MAX_CONFIGURATION_ID; row_id++)
    {
        // TODO: this should be moved to environment control fsm side with getters
        // on measurements data storage side
        if(user_ctx->row_mean_data[row_id].is_row_registered)
        {
            set_row_current_humidity(user_ctx->row_mean_data[row_id].mean_row_humidity, row_id);
            set_row_current_temperature(user_ctx->row_mean_data[row_id].mean_row_temp, row_id);
            set_row_current_light_exposure(user_ctx->row_mean_data[row_id].mean_row_light, row_id);
            set_row_current_soil_moisture(user_ctx->row_mean_data[row_id].mean_row_soil_moisture, row_id);
            set_row_registered(row_id);
        }
        else
        {
            // if a row at a point is not registered, reset its status
            reset_row_status(row_id);
        }
    }

    // Notify environment control fsm that new measurements were taken
    k_event_post(&env_control_event, ENV_CONTROL_MEASUREMENTS_TAKEN_EVT);
    smf_set_state(SMF_CTX(&measurements_fsm_user_object), &measurement_states[SEND_DATA_TO_9160]);
}

// --- SEND_DATA_TO_9160 state ---
static void send_data_to_9160_run(void *o)
{
    struct user_object_s *user_ctx = (struct user_object_s *)o;
    // Message buffers
    message_measurement_data_t msg_measurement_data = {0};
    message_row_mean_data_t msg_row_mean_data = {0};
    message_ready_for_cloud_t msg_ready_for_cloud = {0};

    // TODO: To be done with workqueues
    // Send all measurement data
    for (int index = 0; index < BLE_MAX_CONNECTIONS; index++)
    {
        if (user_ctx->measurements_data[index].ble_connection_handle != NULL)
        {
            create_measurements_data_tx_message(&user_ctx->measurements_data[index], &msg_measurement_data);
            internal_uart_send_data((uint8_t *)&msg_measurement_data, sizeof(message_measurement_data_t));
        }
    }

    // Send row mean data
    for (int index = 0; index < MAX_CONFIGURATION_ID; index++)
    {
        if (user_ctx->row_mean_data[index].is_row_registered)
        {
            create_row_mean_data_tx_message(&user_ctx->row_mean_data[index], &msg_row_mean_data);
            internal_uart_send_data((uint8_t *)&msg_row_mean_data, sizeof(message_row_mean_data_t));
        }
    }

    // Signal 9160 that transfer of measurements is done and it is ready to send them to the cloud
    create_ready_for_cloud_tx_message(&msg_ready_for_cloud);
    internal_uart_send_data((uint8_t *)&msg_ready_for_cloud, sizeof(message_ready_for_cloud_t));

    smf_set_state(SMF_CTX(&measurements_fsm_user_object), &measurement_states[THREAD_SLEEP]);
}

// --- THREAD_SLEEP state ---
static void thread_sleep_run(void *o)
{
    bool is_anyone_connected = false;

    // LOG_INF("---------------------------------------");
    // Measurements are taken with a specific period
    k_sleep(K_MSEC(MEASUREMENT_PERIOD_IN_SEC * 1000));

    // After thread wakes up, check if at least one sensor node is connected. If not, go back to sleep state
    is_anyone_connected = k_sem_count_get(&at_least_one_active_connection_sem) > 0 ? true : false;

    if (is_anyone_connected)
    {
        smf_set_state(SMF_CTX(&measurements_fsm_user_object), &measurement_states[TAKE_MEASUREMENTS]);
    }
    else
    {
        smf_set_state(SMF_CTX(&measurements_fsm_user_object), &measurement_states[THREAD_SLEEP]);
    }
}

void measurements_fsm(void)
{
    int32_t ret;

    k_sem_init(&read_response_sem, 0, 1);
    k_sem_init(&at_least_one_active_connection_sem, 0, BLE_MAX_CONNECTIONS);

    // Set initial state
    smf_set_initial(SMF_CTX(&measurements_fsm_user_object), &measurement_states[THREAD_SLEEP]);

    // Run the state machine
    while (1)
    {
        // Wait for at least one connection to be active in order to start measurement fsm
        k_sem_take(&at_least_one_active_connection_sem, K_FOREVER);
        // give back the semaphore.
        k_sem_give(&at_least_one_active_connection_sem);
        // State machine terminates if a non-zero value is returned
        ret = smf_run_state(SMF_CTX(&measurements_fsm_user_object));
        if (ret)
        {
            // handle return code and terminate state machine
            break;
        }
    }
}

K_THREAD_DEFINE(measurements_fsm_id, STACKSIZE, measurements_fsm, NULL, NULL, NULL,
                PRIORITY, 0, 0);