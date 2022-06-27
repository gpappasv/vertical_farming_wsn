# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_nbiot)

target_sources(app PRIVATE src/main.c internal_uart/internal_uart.c ../common/com_protocol/com_protocol.c internal_uart/message_parsing.c inventory/inventory.c coap_client/coap_client.c coap_client/coap_fsm.c coap_client/coap_message_parsing.c)
