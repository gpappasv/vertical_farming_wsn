// --- includes ----------------------------------------------------------------
#include <zephyr.h>
#include <drivers/watchdog.h>
#include <logging/log.h>
#include "watchdog_timer.h"

// --- logging -----------------------------------------------------------------
LOG_MODULE_REGISTER(wdt_m);

// --- defines -----------------------------------------------------------------
#define WDT_TIMEOUT_VALUE 10000 // Timeout in milliseconds

// --- structs -----------------------------------------------------------------
static const struct device *hw_wdt_dev = DEVICE_DT_GET(DT_ALIAS(watchdog));

static const struct wdt_timeout_cfg wdt_cfg =
    {
        .window.max = WDT_TIMEOUT_VALUE,
        .callback = NULL,
        .flags = WDT_FLAG_RESET_SOC};

// --- static functions declarations -------------------------------------------
static void watchdog_thread(void);

// --- static functions definitions --------------------------------------------
/**
 * @brief Lowest priority application thread to feed the watchdog.
 *        If the thread does not get a chance to execute, it means
 *        some other thread has stuck. Watchdog won't be fed and
 *        thus a reset will be triggered.
 *
 */
static void watchdog_thread(void)
{

    if (hw_wdt_dev)
    {
        while (1)
        {   
            wdt_feed(hw_wdt_dev, 0);
            k_sleep(K_MSEC(WDT_TIMEOUT_VALUE - 100)); // Feed WDT at 9900 msec
            
        }
    }
}

// --- functions declarations -------------------------------------------
/**
 * @brief Watchdog initialization function
 *
 */
void init_watchdog(void)
{

    if (hw_wdt_dev != NULL)
    {
        wdt_install_timeout(hw_wdt_dev, &wdt_cfg);
        wdt_setup(hw_wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG); // Halt WDT during debugging
        wdt_feed(hw_wdt_dev, 0);
    }
    else
    {
        LOG_ERR("Watchdog initialization failed!");
    }
}

K_THREAD_DEFINE(watchdog_tid, 256, watchdog_thread, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 1000);