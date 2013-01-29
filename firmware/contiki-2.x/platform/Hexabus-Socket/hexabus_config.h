/*
Config file for hexabus

Each app that is either optional or has config settings should have a section in this file.
There should be an ENABLE option that enables compilation and execution of the app, a DEBUG option which enables the DEBUG define of the app. You can add more options if your app is configurable somehow.
*/
#ifndef HEXABUS_CONFIG_H
#define HEXABUS_CONFIG_H

// udp_handler
#define UDP_HANDLER_DEBUG 0

#define PACKET_BUILDER_DEBUG 0
#define ENDPOINT_ACCESS_DEBUG 0

// WebServer
#define WEBSERVER_DEBUG 0

// Memory debugger process
#define MEMORY_DEBUGGER_ENABLE 1
// Print a line every MEMORY_DEBUGGER_INTERVAL seconds
#define MEMORY_DEBUGGER_INTERVAL 10

// button
#define BUTTON_DEBUG 0
#define BUTTON_DOUBLE_CLICK_ENABLED 0
#define BUTTON_HAS_EID 1 // set to 1 to have button on EID 4. Set to 0 to have button not do any interaction with network
#define BUTTON_TOGGLES_RELAY 0 // set to 1 to have the button toggle the relay directly

// datetime_service
#define DATETIME_SERVICE_ENABLE 1
#define DATETIME_SERVICE_DEBUG 0

// temperature
#define TEMPERATURE_ENABLE 0
#define TEMPERATURE_DEBUG 0
#define TEMPERATURE_SENSOR 0             // 0 - ds80x20, 1 - HYT321, 2 - BMP085

// value_broadcast
#define VALUE_BROADCAST_ENABLE 1
#define VALUE_BROADCAST_DEBUG 1
#define VALUE_BROADCAST_NUMBER_OF_AUTO_EIDS 1 // Number of endpoints to broadcast automatically - set to 0 to disable
#define VALUE_BROADCAST_AUTO_EIDS 2 // Comma-separated list of endpoints to broadcast automatically
#define VALUE_BROADCAST_AUTO_INTERVAL 20 // Timeout in seconds
#define VALUE_BROADCAST_NUMBER_OF_LOCAL_ONLY_EIDS 0 // Number of endpoints to "broadcast" to the local state machine
#define VALUE_BROADCAST_LOCAL_ONLY_EIDS 2 // Comma-separated list of eids to be sent to local state machine

// metering
#define METERING_IMMEDIATE_BROADCAST 1  // immediately broadcast metering value when change is measured (you still should have the EID in the VALUE_BROADCAST_AUTO_EIDS with some reasonable timeout, so that the value is also broadcast when it reaches and stays at zero)
#define METERING_IMMEDIATE_BROADCAST_NUMBER_OF_TICKS 1 // number of ticks from the meter until a broadcast is triggered. 1: broadcast every tick ~ roughly every 2 seconds at 100W
#define METERING_IMMEDIATE_BROADCAST_MINIMUM_TIMEOUT 20 // minimum number of seconds between two broadcasts, to prevent flooding the network
#define METERING_ENERGY 0
// CAUTION: METERING_ENERGY_PERSISTENT needs external power-down detection circuit. Refer to the Wiki!
#define METERING_ENERGY_PERSISTENT 0 // Persistently store energy value (number of pulses) in EEPROM. 
#define S0_ENABLE 0 //S0 meter instead of internal meter.

// state_machine
#define STATE_MACHINE_ENABLE 1
#define STATE_MACHINE_DEBUG 1

// state machine uploading via Hexabus packets
#define SM_UPLOAD_ENABLE 1

// window blind shutter motor control
#define SHUTTER_ENABLE 0
#define SHUTTER_DEBUG 1
#define SHUTTER_CALIBRATE_ON_BOOT 1
#define SHUTTER_INITIAL_POSITON 1

// hexapush
#define HEXAPUSH_ENABLE 0
#define HEXAPUSH_CLICK_ENABLE 1
#define HEXAPUSH_PRESS_RELEASE_ENABLE 1
#define HEXAPUSH_PRESS_DELAY 6   //multiplied by 50ms, only if Click and Press/Release are enabled
#define HEXAPUSH_DEBUG 0

// presence detector
#define PRESENCE_DETECTOR_ENABLE 0
#define PRESENCE_DETECTOR_DEBUG 0
#define PRESENCE_DETECTOR_SERVER 0
#define PRESENCE_DETECTOR_CLIENT 1
#define PRESENCE_DETECTOR_SERVER_TIMEOUT 3 // in Minutes
#define PRESENCE_DETECTOR_CLIENT_GROUP 2 // 2 to 255
#define PRESENCE_DETECTOR_CLIENT_KEEP_ALIVE 60 // in seconds, 0 to disable

// hexonoff
#define HEXONOFF_ENABLE 0
#define HEXONOFF_DEBUG 0
#define HEXONOFF_INITIAL_VALUE 0

//Lightsensor (only works in combination with analogread!)
#define LIGHTSENSOR_ENABLE 0

// read analog input pin
#define ANALOGREAD_ENABLE 0
#define ANALOGREAD_DEBUG 1
#define ANALOGREAD_PIN 0 // 0 to 7
#define ANALOGREAD_MULT 0.0024414062 // readings are multiplied with this value to calculate the value sent to the endpoint. Set to 0.0024414062 to get the Voltage reading (in Volts) at 2.5V supply voltage

//i2c master
#define I2C_ENABLE 0
#define I2C_DEBUG 0

//humidity sensor
#define HUMIDITY_ENABLE 0
#define HUMIDITY_DEBUG 0

//pressure sensor
#define PRESSURE_ENABLE 0
#define PRESSURE_DEBUG 0
#define PRESSURE_OVERSAMPLING 3  //0 to 3

//ir_receiver
#define IR_RECEIVER_ENABLE 0
#define IR_RECEIVER_DEBUG 0
#define IR_REPEAT 1                  // 0 disables repeat, 1 enables repeat for buttons configured in ir_receiver.h
#define IR_RECEIVER_RAW_MODE 0

#endif // HEXBAUS_CONFIG_H
