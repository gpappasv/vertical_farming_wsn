/*
 * Description:
 *
 * Source file to find and subscribe (enable notifications) to a desired
 * characteristic
 *
 */

// --- includes ----------------------------------------------------------------
#include "ble_characteristic_control.h"
#include "ble_fsm.h"
#include "ble_connection_data.h"
#include "../measurements/measurements_data_storage.h"
#include "../../common/common.h"
#include <logging/log.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_DECLARE(ble_m);

// --- defines -----------------------------------------------------------------
// service map contains all the supported service UUIDs
#define SERVICE_MAP_SIZE 2
// characteristic map contains all the supported characteristic UUIDs
#define CHARACTERISTIC_MAP_SIZE MEASUREMENT_SERVICE_CHARACTERISTIC_COUNT + CONFIGURE_SERVICE_CHARACTERISTIC_COUNT

// --- static variables definitions --------------------------------------------
static struct bt_gatt_discover_params discover_params = {0};
static struct bt_gatt_read_params read_parameters = {0};

// --- service and characteristic map: here we store all supported services UUIDs and all supported characteristic UUIDs
// those maps help us to reference a service or a characteristic by an index and not by UUIDs
static struct bt_uuid *service[SERVICE_MAP_SIZE] = {BT_UUID_MEASUREMENT_SERVICE, BT_UUID_CONFIGURE_SERVICE};
static struct bt_uuid *characteristic[CHARACTERISTIC_MAP_SIZE] = {BT_UUID_TEMPERATURE, BT_UUID_HUMIDITY, BT_UUID_SOIL_MOISTURE, BT_UUID_LIGHT_EXPOSURE, BT_UUID_CONFIGURATION, BT_UUID_BAS_BATTERY_LEVEL};
static uint8_t service_index;
static uint8_t characteristic_index;

// --- static function declarations --------------------------------------------
static uint8_t characteristic_discovery(struct bt_conn *conn,
                                        const struct bt_gatt_attr *attr,
                                        struct bt_gatt_discover_params *params);

static uint8_t read_characteristic_cb(struct bt_conn *conn, uint8_t err,
                                      struct bt_gatt_read_params *params,
                                      const void *data, uint16_t length);
static void store_value_handle(struct bt_conn *conn, uint16_t value_handle, uint8_t characteristic);

// --- static function definitions ---------------------------------------------
/**
 * @brief Function that discovers characteristic handle. This function is actually
 *        a callback that characteristic_discovery_wrapper calls after setting the
 *        desired service and characteristic that should be discoverd and save
 *        its handle value
 *
 * @param conn Connection handle
 * @param attr
 * @param params
 * @return uint8_t Error code
 */
static uint8_t characteristic_discovery(struct bt_conn *conn,
                                        const struct bt_gatt_attr *attr,
                                        struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr)
    {
        LOG_INF("Discover complete");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    if (!bt_uuid_cmp(discover_params.uuid, service[service_index]))
    {
        // TODO: should use "*params" probably. discover_params is constructed on
        // characteristic_discovery_wrapper and  characteristic_discovery is called as
        // a cb function with discovery_params as params
        discover_params.uuid = characteristic[characteristic_index];
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err)
        {
            remove_connection_data(conn);
            LOG_INF("Discover failed (err %d)", err);
        }
    }
    else if (!bt_uuid_cmp(discover_params.uuid, characteristic[characteristic_index]))
    {
        // --- store characteristic handle
        store_value_handle(conn, bt_gatt_attr_value_handle(attr), characteristic_index);

        k_sem_give(&ble_char_discovery_sem);

        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_STOP;
}

/**
 * @brief Callback function that performs ble read on a desired characteristic
 *        of a connected device.
 *        TODO: probably add the measurements_data_storage setter in here to
 *              store the value
 *
 * @param conn
 * @param err
 * @param params
 * @param data
 * @param length
 * @return uint8_t
 */
static uint8_t read_characteristic_cb(struct bt_conn *conn, uint8_t err,
                                      struct bt_gatt_read_params *params,
                                      const void *data, uint16_t length)
{
    // Measured data are casted to 32 bit integer, we might have another var type for
    // each characteristic
    int32_t *measurement_data = (int32_t *)data;
    // fetch the ble_connection_data bluetooth_devices that corresponds to the
    // given connection handle
    ble_connection_data_t *conn_data = get_device_by_conn_handle(conn);
    // if nothing fetched (invalid connection handle), get_device_by_conn_handle()
    // will return null
    if (conn_data == NULL)
    {
        LOG_INF("Error in connection handle");
        return BT_GATT_ITER_STOP;
    }

    // TODO: define this error state (describe it better)
    if (data == NULL)
    {
        LOG_INF("Read complete");
        return BT_GATT_ITER_STOP;
    }

    // printk for now -> will be replaced with measurements setter
    if (params->single.handle == conn_data->temperature_value_handle)
    {
        // Store measured temperature on measurement_data
        set_temperature_measurement_value(conn, *measurement_data);
    }
    else if (params->single.handle == conn_data->humidity_value_handle)
    {
        set_humidity_measurement_value(conn, *measurement_data);
    }
    else if(params->single.handle == conn_data->soil_moisture_value_handle)
    {
        set_soil_moisture_measurement_value(conn, *measurement_data);
    }
    else if(params->single.handle == conn_data->light_intensity_value_handle)
    {
        set_light_intensity_measurement_value(conn, *measurement_data);
    }
    else if (params->single.handle == conn_data->configuration_value_handle)
    {
        set_configuration_id_value(conn, (uint8_t)*measurement_data);
    }
    else if (params->single.handle == conn_data->battery_value_handle)
    {
        set_battery_level_value(conn, (uint8_t)*measurement_data);
    }

    return BT_GATT_ITER_STOP;
}

/**
 * @brief Function that stores the handle value of the discovered characteristic
 *        on the corresponding bluetooth_device
 *
 * @param conn
 * @param value_handle
 * @param characteristic
 */
static void store_value_handle(struct bt_conn *conn, uint16_t value_handle, uint8_t characteristic)
{
    ble_connection_data_t *conn_data;
    conn_data = get_device_by_conn_handle(conn);

    if (conn_data == NULL)
    {
        LOG_INF("Invalid connection handle");
        return;
    }

    switch (characteristic)
    {
    case TEMPERATURE_CHAR_INDEX:
        conn_data->temperature_value_handle = value_handle;
        break;
    case HUMIDITY_CHAR_INDEX:
        conn_data->humidity_value_handle = value_handle;
        break;
    case SOIL_MOISTURE_CHAR_INDEX:
        conn_data->soil_moisture_value_handle = value_handle;
        break;
    case LIGHT_INTENSITY_CHAR_INDEX:
        conn_data->light_intensity_value_handle = value_handle;
        break;
    case CONFIGURATION_CHAR_INDEX:
        conn_data->configuration_value_handle = value_handle;
        break;
    case BATTERY_CHAR_INDEX:
        conn_data->battery_value_handle = value_handle;
        break;
    default:
        LOG_INF("Invalid characteristic value: %d", characteristic);
        break;
    }
}

// --- function definitions ----------------------------------------------------
/**
 * @brief Function that constructs discovery params, selects a desired service and characteristic
 *        and calls characteristic_discovery to discover (and store) the characteristic handle value
 *        of the selected service existing on the connection handle
 *        TODO: as more services or characteristics are added, the switch cases should be updated
 *              just like the static struct bt_uuid *service, static struct bt_uuid *characteristic
 *
 * @param conn Connection handle
 * @param service_select Service selection index (e.g. see MEASUREMENT_SERVICE_INDEX)
 * @param char_select Characteristic selection index (e.g. see TEMPERATURE_CHAR_INDEX)
 */
void characteristic_discovery_wrapper(struct bt_conn *conn, uint8_t service_select, uint8_t char_select)
{
    int err;

    // Set the index of the array: static struct bt_uuid *service to select the
    // desired service UUID
    // TODO: probably include the characteristic switch inside the service
    // switch (case MEASUREMENT_SERVICE_INDEX)
    //              ...
    //              case TEMPERATURE_CHAR_INDEX ....
    switch (service_select)
    {
    case MEASUREMENT_SERVICE_INDEX:
        service_index = MEASUREMENT_SERVICE_INDEX;
        // Set the index of the array: static struct bt_uuid *characteristic to
        // select the desired characteristic UUID
        switch (char_select)
        {
        case TEMPERATURE_CHAR_INDEX:
            characteristic_index = TEMPERATURE_CHAR_INDEX;
            break;
        case HUMIDITY_CHAR_INDEX:
            characteristic_index = HUMIDITY_CHAR_INDEX;
            break;
        case SOIL_MOISTURE_CHAR_INDEX:
            characteristic_index = SOIL_MOISTURE_CHAR_INDEX;
            break;
        case LIGHT_INTENSITY_CHAR_INDEX:
            characteristic_index = LIGHT_INTENSITY_CHAR_INDEX;
            break;
        default:
            // --- Add debug info
            LOG_INF("Invalid measurement characteristic selection: %d", char_select);
            return;
        }
        break;
    case CONFIGURE_SERVICE_INDEX:
        service_index = CONFIGURE_SERVICE_INDEX;
        // Set the index of the array: static struct bt_uuid *characteristic to
        // select the desired characteristic UUID
        switch (char_select)
        {
        case CONFIGURATION_CHAR_INDEX:
            characteristic_index = CONFIGURATION_CHAR_INDEX;
            break;
        case BATTERY_CHAR_INDEX:
            characteristic_index = BATTERY_CHAR_INDEX;
            break;
        default:
            // --- Add debug info
            LOG_INF("Invalid measurement characteristic selection: %d", char_select);
            return;
        }
        break;
    default:
        // --- Add debug info
        LOG_INF("Invalid service selection: %d", service_select);
        return;
        break;
    }

    // --- Discover characteristic parameters
    discover_params.uuid = service[service_index];
    discover_params.func = characteristic_discovery;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(conn, &discover_params);
    if (err)
    {
        LOG_INF("Discover failed(err %d)", err);
        return;
    }
}

/**
 * @brief Wrapper function to read a characteristic value of a connected devices that
 *        corresponds to the @param: conn.
 *        The caracteristic to be read is referenced by an index (see TEMPERATURE_CHAR_INDEX)
 *
 * @param conn Connection handle
 * @param char_select Takes values like: TEMPERATURE_CHAR_INDEX
 */
void read_characteristic_wrapper(struct bt_conn *conn, uint8_t char_select)
{
    uint16_t characteristic_handle;

    // fetch the ble_connection_data bluetooth_devices that corresponds to the
    // given connection handle
    ble_connection_data_t *conn_data = get_device_by_conn_handle(conn);

    // if nothing fetched (invalid connection handle), get_device_by_conn_handle()
    // will return null
    if (conn_data == NULL)
    {
        LOG_INF("Invalid connection handle");
        return;
    }

    // Select one of the available characteristics of the selected connected device
    // TODO: This switch will get bigger as more characteristics are added to the measurement service
    switch (char_select)
    {
    // --- measurement service
    case TEMPERATURE_CHAR_INDEX:
        characteristic_handle = conn_data->temperature_value_handle;
        break;
    case HUMIDITY_CHAR_INDEX:
        characteristic_handle = conn_data->humidity_value_handle;
        break;
    case SOIL_MOISTURE_CHAR_INDEX:
        characteristic_handle = conn_data->soil_moisture_value_handle;
        break;
    case LIGHT_INTENSITY_CHAR_INDEX:
        characteristic_handle = conn_data->light_intensity_value_handle;
        break;
    // --- configure service
    case CONFIGURATION_CHAR_INDEX:
        characteristic_handle = conn_data->configuration_value_handle;
        break;
    case BATTERY_CHAR_INDEX:
        characteristic_handle = conn_data->battery_value_handle;
        break;
    default:
        // --- Add debug info
        LOG_INF("Invalid characteristic selection: %d", char_select);
        return;
    }
    // --- Construct the read parameters
    read_parameters.handle_count = 1;
    read_parameters.single.handle = characteristic_handle;
    read_parameters.func = read_characteristic_cb;

    // Read characteristic request
    bt_gatt_read(conn, &read_parameters);
}