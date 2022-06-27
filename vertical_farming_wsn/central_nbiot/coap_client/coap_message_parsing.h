#ifndef COAP_MESSAGE_PARSING_H
#define COAP_MESSAGE_PARSING_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// --- functions declarations --------------------------------------------------
uint8_t process_coap_rx_message(const uint8_t *rx_buf);

#endif // COAP_MESSAGE_PARSING_H