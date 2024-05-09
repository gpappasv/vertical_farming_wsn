#ifndef FLASH_SYSTEM_H
#define FLASH_SYSTEM_H

// --- includes ----------------------------------------------------------------
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

// --- defines -----------------------------------------------------------------
// --- defines -----------------------------------------------------------------
#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define DEVICE_CONFIGURATION_FLASH_KEY 0
#define SOIL_MOISTURE_DRY_CONFIG_FLASH_KEY 1
#define SOIL_MOISTURE_WET_CONFIG_FLASH_KEY 2
// --- functions declarations --------------------------------------------------
void flash_system_init(void);
struct nvs_fs* get_file_system_handle(void);

#endif // FLASH_SYSTEM_H
