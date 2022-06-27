# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_node)

target_sources(app PRIVATE 
main.c 
ble_client/ble_fsm.c 
ble_client/ble_conn_control.c 
ble_client/ble_characteristic_control.c 
ble_client/ble_connection_data.c 
measurements/measurements_fsm.c 
measurements/measurements_data_storage.c 
internal_uart/internal_uart.c 
../common/com_protocol/com_protocol.c 
internal_uart/message_parsing.c 
watchdog_timer/watchdog_timer.c
flash_system/flash_system.c
environment_control/environment_control_fsm.c
environment_control/environment_control_config.c
)
