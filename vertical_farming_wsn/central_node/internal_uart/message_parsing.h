// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// --- functions declarations --------------------------------------------------
bool is_message_type_valid(uint8_t type);
uint8_t process_rx_message(uint8_t *rx_buf);