#ifndef MESSAGE_PARSING_H
#define MESSAGE_PARSING_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// --- functions declarations --------------------------------------------------
uint8_t process_rx_message(uint8_t *rx_buf);
bool is_message_type_valid(uint8_t type);

#endif // MESSAGE_PARSING_H