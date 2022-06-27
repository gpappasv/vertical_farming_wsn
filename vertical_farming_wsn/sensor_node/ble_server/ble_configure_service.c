// --- includes ----------------------------------------------------------------
#include "ble_configure_service.h"
#include "../flash_system/flash_system.h"
#include "../../common/common.h"

#include <logging/log.h>
#include <stdint.h>

#include <bluetooth/addr.h>
#include <bluetooth/gatt.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(ble_m);

// --- extern variables declarations -------------------------------------------
extern struct k_sem configure_done_sem;

// --- static functions declarations -------------------------------------------
static ssize_t configuration_write_id(struct bt_conn *conn,
                          const struct bt_gatt_attr *attr,
                          const void *buf,
                          uint16_t len,
                          uint16_t offset,
                          uint8_t flags);

// --- static functions definitions --------------------------------------------
/* This function is called whenever the RX Characteristic has been written to by a Client */
static ssize_t configuration_write_id(struct bt_conn *conn,
                          const struct bt_gatt_attr *attr,
                          const void *buf,
                          uint16_t len,
                          uint16_t offset,
                          uint8_t flags)
{
    int err;
    const uint8_t *configuration_id = buf;

    if(*configuration_id > MAX_CONFIGURATION_ID || *configuration_id == 0)
    {
        // TODO: Should reject the written value and return to configuration state
        LOG_INF("Invalid configuration ID: %d", *configuration_id);
        return 0;
    }

    // Write Configuration ID to flash
    err = nvs_write(get_file_system_handle(), DEVICE_CONFIGURATION_FLASH_KEY, configuration_id, sizeof(*configuration_id));
    if(err<0)
    {
        LOG_INF("NVS write failed (err: %d)", err);
    }
    else
    {
        // After configuration is done, give configuration semaphore
        // TODO: this semaphore is given every time we write on the configuration characteristic
        k_sem_give(&configure_done_sem);
    }
    return len;
}

static ssize_t on_read_battery_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset)
{
    
    int32_t battery_level = 64;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &battery_level,
                             sizeof(battery_level));
}

static ssize_t on_read_configuration_id(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset)
{
    
    uint8_t configuration_id;

    // Read stored configuration id
    nvs_read(get_file_system_handle(), DEVICE_CONFIGURATION_FLASH_KEY, &configuration_id, sizeof(configuration_id));

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &configuration_id,
                             sizeof(configuration_id));
}

// --- functions definitions ---------------------------------------------------
BT_GATT_SERVICE_DEFINE(configuration_service, BT_GATT_PRIMARY_SERVICE(BT_UUID_CONFIGURE_SERVICE),
                       BT_GATT_CHARACTERISTIC(BT_UUID_CONFIGURATION, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ, BT_GATT_PERM_WRITE | BT_GATT_PERM_READ, on_read_configuration_id, configuration_write_id, NULL),
                       BT_GATT_CCC(NULL, BT_GATT_PERM_WRITE),
                       BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, on_read_battery_level, NULL, NULL),
                       BT_GATT_CCC(NULL, BT_GATT_PERM_READ) );