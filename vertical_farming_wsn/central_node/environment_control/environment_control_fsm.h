#ifndef ENVIRONMENT_CONTROL_FSM_H
#define ENVIRONMENT_CONTROL_FSM_H

// --- includes ----------------------------------------------------------------

// --- defines -----------------------------------------------------------------
#define ENV_CONTROL_STACKSIZE 1024
#define ENV_CONTROL_PRIORITY  8

// --- enums -------------------------------------------------------------------
typedef enum
{
    ENV_CONTROL_MEASUREMENTS_TAKEN_EVT = 0x001,
    ENV_CONTROL_USER_REQUEST_EVENT = 0x002
}env_control_events_e;

#endif // ENVIRONMENT_CONTROL_FSM_H