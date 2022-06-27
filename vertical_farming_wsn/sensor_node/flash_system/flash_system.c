// --- includes ----------------------------------------------------------------
#include "flash_system.h"
#include <logging/log.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(flash_system_m);

// --- static variables definitions --------------------------------------------
static struct nvs_fs fs;
static const struct device *flash_dev;

// --- functions definitions ---------------------------------------------------
void flash_system_init(void)
{
    int err = 0;
    struct flash_pages_info info;

    flash_dev = FLASH_AREA_DEVICE(STORAGE_NODE_LABEL);
    if (!device_is_ready(flash_dev)) 
    {
        LOG_INF("Flash device %s is not ready\n", flash_dev->name);
        return;
    }

	fs.offset = FLASH_AREA_OFFSET(storage);
	err = flash_get_page_info_by_offs(flash_dev, fs.offset, &info);
	if (err) {
		LOG_INF("Unable to get page info\n");
		return;
	}
    fs.sector_size = info.size;
    fs.sector_count = 3U;

    err = nvs_init(&fs, flash_dev->name);
    if (err) {
        LOG_INF("Flash Init failed\n");
        return;
    }
}

// File system handle getter
struct nvs_fs* get_file_system_handle(void)
{
    return &fs;
}