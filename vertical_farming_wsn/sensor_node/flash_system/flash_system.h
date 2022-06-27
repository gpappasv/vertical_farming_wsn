#ifndef FLASH_SYSTEM_H
#define FLASH_SYSTEM_H

// --- includes ----------------------------------------------------------------
#include <zephyr.h>
#include <sys/reboot.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <fs/nvs.h>

// --- defines -----------------------------------------------------------------
#define STORAGE_NODE_LABEL storage
#define DEVICE_CONFIGURATION_FLASH_KEY 0

// --- functions declarations --------------------------------------------------
void flash_system_init(void);
struct nvs_fs* get_file_system_handle(void);

#endif // FLASH_SYSTEM_H
