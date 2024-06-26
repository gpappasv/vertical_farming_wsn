# Description of repository
This is a repository where a Bluetooth Low Energy wireless sensor network is being developed.
The sensor network is being built using the Nordic's nRF chips.

# Vertical Farming Wireless Sensor Network

This is the main repo for the vertical farming WSN solution.

***projects***: all original vertical farming solution code.

***scripts***: helper scripts.

# Project description
This is a wireless sensor network project for farming installation monitoring and automation.

The key elements of the architecture are:

**Sensor nodes**: Sensor nodes are low-power, battery powered nodes that have the relevant sensor installed on them.
A sensor node equips a soil moisture sensor, an environment temperature/humidity sensor and a light intensity sensor.

**Central node**: Central node is a main-powered node that coordinates the network, gathers the measurements from the sensor nodes, posts them to an online server and performs the central management (automation) of the farming installation (for example: a fan, the lights, a watering system). Currently the automation part is done via GPIO pins with simple on/off of the external devices.

**Cloud Server**: The central node posts the measurements and interacts (receives configuration data) with an online server. This server consists of a CoAP server, a webserver and a database.

Technical details of the system:

**Sensor nodes**: 
- The sensor nodes should be first configured by the user, in order to be connected to the central node. The user should configure the row_id of the sensor node to indicate which group is this sensor node part of. (values accepted 1 - 5)
- The sensor nodes are built on **nRF52840 SoC**.
- The communication with the central node happens via **BLE protocol**.
- It's firmware is written using **Zephyr RTOS**.

**Central node**:
- The central node is implemented on nRF9160DK. The nRF52840 subsystem is the WSN master/coordinator. The BLE connection happens from this subsystem. The nRF9160 subsystem is just the communication middleman with the cloud. nRF9160 supports **NB-IoT** connectivity. nRF52840 subsystem forwards data to nRF9160, who then forwards them to the cloud server. Also the nRF9160 subsystem will receive data/commands from the cloud server, that will forward to the nRF52840 subsystem.
- Supports a theoretical maximum of 20 sensor node connections.
- The communication with the cloud server happens via **CoAP protocol**.
- It's firmware is written using **Zephyr RTOS**.

Newer version of Central node is now released. The central node is now implemented on the nRF7002DK board. This board has WiFi capabilities. Now the central node duties are carried out by just one firmware that merges both the central wireless sensor network functionalities (to handle the BLE network) and the cloud communication, via WiFi.

**Cloud server**:
- CoAP server is implemented in Python, using aiocoap library.
- The webserver is a simple flask python webserver.
- The database serves as a data storage element, as well as the way for the CoAP server and web server to communicate with each other.
- TODO: The python implementation of the cloud server is really bad (codewise). Almost unreadable, with many hardcoded things. This was due to limited available time and strict deadlines. Scrips should be reworked!!!

**System Overview**:

Below there is a system overview providing the high level communication between the subsystems of the wireless sensor network.
![image](https://github.com/gpappasv/verticalFarmingWSN/assets/22116014/4537abfa-1d95-43c6-a056-a653c649559f)




_TODO: This documentation just gives a brief overview of the system. A lot of things should be added for a complete documentation. **WIP!**_
_TODO: Should provide instructions on how to deploy the project **WIP!**_
_TODO: Should provide a demo **WIP!**_
