// --- includes ----------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>

// --- defines -----------------------------------------------------------------
#define UART_BUFFER 1024
// size of stack area used by each thread
#define STACKSIZE 1024
// scheduling priority used by each thread
// TODO: probably add a central headerfile with priorities
#define PRIORITY 7

// --- enums -------------------------------------------------------------------
typedef enum
{
    MESSAGE_TO_REPLY_EVT = 0x001,
    COAP_ROW_DATA_TO_SERVER_EVT = 0x002
}internal_uart_evts_e;

// --- functions declarations --------------------------------------------------
void internal_uart_init(void);
int internal_uart_send_data(const uint8_t *buf, size_t size);
void send_operation_result(uint8_t status);