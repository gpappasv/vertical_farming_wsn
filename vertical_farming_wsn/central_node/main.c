// --- includes ----------------------------------------------------------------
#include "internal_uart/internal_uart.h"
#include "watchdog_timer/watchdog_timer.h"
#include "flash_system/flash_system.h"

// --- functions definitions ---------------------------------------------------
void main(void)
{
    // Init watchdog init
    init_watchdog();
    // Initialize internal uart (to communicate with 9160)
    internal_uart_init();
    flash_system_init();
}