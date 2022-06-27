// TODO: create simple flowcharts of each module and add a description on top of every source files (maybe on wiki pages)
// TODO: should connect to devices that support all services we need (measurement service, configuration service etc)
// TODO: Add documentation (on wiki page) for the semaphores (like a flowchart)
// TODO: use events instead of semaphores where possible
// TODO: Use mutex for internal_uart_send_data
// TODO: Replace LOG_INF by LOG_ERR and better utilize logging of zephyr
// TODO: cleanup data types (for example, temperature/mean_temperature is int32_t at some places and int16_t at other places)
// TODO: cleanup measurement_data from ble connection handles and keep only the mac address
// TODO: utilize better the uart send operation result.. Currently does nothing
// TODO: check and utilize crc on communication (uart/coap)
// TODO: Maybe delete mac address from measurement data and add a separate field on the measurement message for mac address
// fetch it via a function through ble connection handle. Also add fields for battery of the device on the message.
// TODO: for mean_row_measurements and measurement_data, that exist on measurements_data_storage, use getters and setters instead of
// get_row_mean_data and get_measurements_data. After that, cleanup the measurements_fsm. Use the getters and setters in order to update
// the measurements_data_storage. 
// TODO: is it ok that measurements fsm updates the environment_control_config?
