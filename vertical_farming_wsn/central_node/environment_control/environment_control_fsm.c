// --- includes ----------------------------------------------------------------
#include "environment_control_fsm.h"
#include "environment_control_config.h"
#include "../../common/common.h"
#include <smf.h>
#include <logging/log.h>
#include <zephyr.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(environment_control_m);

// --- enums -------------------------------------------------------------------
// List of states
enum ble_state_e
{
    ENV_CONTROL_INIT,
    ENV_CONTROL,
    ENV_CONTROL_WFE
};

// --- structs -----------------------------------------------------------------
// User defined object
struct user_object_s
{
    // This must be first
    struct smf_ctx ctx;
} env_control_fsm_user_object;

// --- static function declarations --------------------------------------------
static void env_control_init_run(void *o);

static void env_control_run(void *o);

static void env_control_wfe_run(void *o);

// --- static variables definitions --------------------------------------------
// Populate state table
static const struct smf_state env_control_states[] = {
    [ENV_CONTROL_INIT] = SMF_CREATE_STATE(NULL, env_control_init_run, NULL),
    [ENV_CONTROL] = SMF_CREATE_STATE(NULL, env_control_run, NULL),
    [ENV_CONTROL_WFE] = SMF_CREATE_STATE(NULL, env_control_wfe_run, NULL),
};

// --- variables definitions ---------------------------------------------------
struct k_event env_control_event;

// --- static function definitions ---------------------------------------------
// --- ENV_CONTROL_INIT state ---
static void env_control_init_run(void *o)
{
    k_event_init(&env_control_event);
    // load default if nothing stored
    // TODO: first check if there is a configuration stored in flash
    initialize_row_control_configuration();
    smf_set_state(SMF_CTX(&env_control_fsm_user_object), &env_control_states[ENV_CONTROL_WFE]);
}

// --- ENV_CONTROL state ---
static void env_control_run(void *o)
{
    // Control the fans/lights/water row by row
    // Check if automatic control is enabled -> if enabled, check the threshold
    // and control fan/light/water
    // If automatic control is disabled, blindly do what user has set for fan/light/water
    for(uint8_t row_index = 0; row_index < MAX_CONFIGURATION_ID; row_index++)
    {
        // Check if row is registered, if not, turn fan/light/water off
        if(get_row_registered(row_index))
        {
            // Check if manual control for the corresponding row is enabled
            if(!get_row_automatic_control(row_index))
            {
                // Control the fan
                if(get_row_fan_switch(row_index))
                {
                    // Set fan on for row index
                    LOG_INF("Manual fan on for %d", row_index + 1);
                }
                else
                {
                    // Set fan off for row index
                    LOG_INF("Manual fan off for %d", row_index + 1);
                }

                // Control the water
                if(get_row_water_switch(row_index))
                {
                    // Set water on for row index
                    LOG_INF("Manual water on for %d", row_index + 1);
                }
                else
                {
                    // Set water off for row index
                    LOG_INF("Manual water off for %d", row_index + 1);
                }

                // Control the lights
                if(get_row_light_switch(row_index))
                {
                    // Set lights on for row index
                    LOG_INF("Manual lights on for %d", row_index + 1);
                }
                else
                {
                    // Set lights off for row index
                    LOG_INF("Manual lights off for %d", row_index + 1);
                }
            }
            else // if automatic control
            {
                // Control the fan
                if((get_row_current_temperature(row_index) > get_row_temp_threshold(row_index)) || (get_row_current_humidity(row_index) > get_row_hum_threshold(row_index)))
                {
                    // Set fan on for row index
                    LOG_INF("Auto fan on for %d", row_index + 1);
                }
                else
                {
                    // Set fan off for row index
                    LOG_INF("Auto fan off for %d", row_index + 1);
                }

                // Control the water
                if(get_row_current_soil_moisture(row_index) < get_row_soil_moisture_threshold(row_index))
                {
                    // Set water on for row index
                    LOG_INF("Auto water on for %d", row_index + 1);
                }
                else
                {
                    // Set water off for row index
                    LOG_INF("Auto water off for %d", row_index + 1);
                }

                // Control the lights
                if(get_row_current_light_exposure(row_index) < get_row_light_threshold(row_index))
                {
                    // Set lights on for row index
                    LOG_INF("Auto lights on for %d", row_index + 1);
                }
                else
                {
                    // Set lights off for row index
                    LOG_INF("Auto lights off for %d", row_index + 1);
                }
            }
        }
        else
        {
            // Set fan/water/light off
            LOG_INF("Row not registered, so all off %d", row_index + 1);
        }
    }
    smf_set_state(SMF_CTX(&env_control_fsm_user_object), &env_control_states[ENV_CONTROL_WFE]);
}

// --- ENV_CONTROL_WFE state ---
static void env_control_wfe_run(void *o)
{
    uint32_t events;
    events = k_event_wait(&env_control_event, ENV_CONTROL_MEASUREMENTS_TAKEN_EVT | ENV_CONTROL_USER_REQUEST_EVENT, true, K_FOREVER);
    smf_set_state(SMF_CTX(&env_control_fsm_user_object), &env_control_states[ENV_CONTROL]);
}

void env_control_fsm(void)
{
    int32_t ret;

    // Set initial state
    smf_set_initial(SMF_CTX(&env_control_fsm_user_object), &env_control_states[ENV_CONTROL_INIT]);

    // Run the state machine
    while (1)
    {
        // State machine terminates if a non-zero value is returned
        ret = smf_run_state(SMF_CTX(&env_control_fsm_user_object));
        if (ret)
        {
            // handle return code and terminate state machine
            break;
        }
    }
}

K_THREAD_DEFINE(env_control_fsm_id, ENV_CONTROL_STACKSIZE, env_control_fsm, NULL, NULL, NULL,
                ENV_CONTROL_PRIORITY, 0, 0);