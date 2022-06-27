// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>

// --- defines -----------------------------------------------------------------
#define UART_BUFFER 1024

// --- functions declarations --------------------------------------------------
void internal_uart_init(void);
int internal_uart_send_data(const uint8_t *buf, size_t size);