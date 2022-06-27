#ifndef INVENTORY_H
#define INVENTORY_H

// --- includes ----------------------------------------------------------------
#include "../../common/com_protocol/com_protocol.h"

// --- functions declarations --------------------------------------------------
uint8_t store_measurement_message(message_measurement_data_t *msg_to_store);
uint8_t store_row_mean_data_message(message_row_mean_data_t *msg_to_store);
void reset_measurements_inventory(void);
void reset_row_mean_data_inventory(void);
row_mean_data_t* get_row_mean_data_inventory(void);
measurements_data_t *get_measurements_data_inventory(void);

#endif // INVENTORY_H