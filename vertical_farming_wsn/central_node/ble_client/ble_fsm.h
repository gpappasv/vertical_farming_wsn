#ifndef BLE_FSM_H
#define BLE_FSM_H

// --- defines -----------------------------------------------------------------
// size of stack area used by each thread
#define STACKSIZE 1024
// scheduling priority used by each thread
#define PRIORITY 7

// --- extern variables declarations -------------------------------------------
extern struct k_sem ble_init_ok_sem;
extern struct k_sem ble_connect_ok_sem;
extern struct k_sem ble_char_discovery_sem;
extern struct k_sem ble_wait_for_disconnect_sem;

#endif // BLE_FSM_H
