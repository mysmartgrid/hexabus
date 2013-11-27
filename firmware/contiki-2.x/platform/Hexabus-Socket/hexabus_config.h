/*
Config file for hexabus

Each app that is either optional or has config settings should have a section in this file.
There should be an ENABLE option that enables compilation and execution of the app, a DEBUG option which enables the DEBUG define of the app. You can add more options if your app is configurable somehow.

_DEBUG options should be set to:
 * 0 to disable debug output
 * one of the LOG_* levels in syslog.h to log everything of higher or equal priority
 * LOG_DEBUG, 9, or a larger integer to log everything
*/
#ifndef HEXABUS_CONFIG_H
#define HEXABUS_CONFIG_H

// options common to all incarnations of Hexabus devices

#define UDP_HANDLER_DEBUG 0

#define ENDPOINT_REGISTRY_DEBUG 0

// WebServer
#define WEBSERVER_DEBUG 0

// Memory debugger process
#define MEMORY_DEBUGGER_ENABLE 0
// Print a line every MEMORY_DEBUGGER_INTERVAL seconds
#define MEMORY_DEBUGGER_INTERVAL 10

// button
#define BUTTON_HAS_EID 1 // set to 1 to have button on EID 4. Set to 0 to have button not do any interaction with network
#define BUTTON_TOGGLES_RELAY 0 // set to 1 to have the button toggle the relay directly

#define BUTTON_DEBOUNCE_TICKS		1
#define BUTTON_PIN							PIND
#define BUTTON_BIT							PD5
#define	BUTTON_CLICK_MS					2000UL
#define	BUTTON_LONG_CLICK_MS		7000UL

// datetime_service
#define DATETIME_SERVICE_ENABLE 1
#define DATETIME_SERVICE_DEBUG 0

// state_machine
#define STATE_MACHINE_ENABLE 1
#define STATE_MACHINE_DEBUG 0

// state machine uploading via Hexabus packets
#define SM_UPLOAD_ENABLE 1 



// Device profiles. Choose a profile for the device you want to build or add a new profile.
#define HXB_PROFILE_PLUG            1
#define HXB_PROFILE_PLUGPLUS        2
#define HXB_PROFILE_EMOS            3
#define HXB_PROFILE_TEMPSENSOR      4
#define HXB_PROFILE_HUMIDITYSENSOR  5
#define HXB_PROFILE_PRESSURESENSOR  6

#define HXB_DEVICE_PROFILE   HXB_PROFILE_PLUG

#define HAVE(x) (HXB_DEVICE_PROFILE == HXB_PROFILE_ ## x)


// temperature
#define TEMPERATURE_ENABLE (HAVE(EMOS) || HAVE(TEMPSENSOR) || HAVE(HUMIDITYSENSOR))
#define TEMPERATURE_DEBUG 0
#define TEMPERATURE_SENSOR (HAVE(EMOS) || HAVE(HUMIDITYSENSOR) ? 1 : 0) // 0 - ds80x20, 1 - HYT321, 2 - BMP085

// value_broadcast
#define VALUE_BROADCAST_ENABLE 1
#define VALUE_BROADCAST_DEBUG 0
#define VALUE_BROADCAST_AUTO_INTERVAL 20 // Timeout in seconds
#if HAVE(PLUG)
 #define VALUE_BROADCAST_AUTO_EIDS EP_POWER_SWITCH
#elif HAVE(PLUGPLUS)
 #define VALUE_BROADCAST_AUTO_EIDS EP_POWER_SWITCH, EP_POWER_METER
#elif HAVE(EMOS)
 #define VALUE_BROADCAST_AUTO_EIDS EP_TEMPERATURE, EP_HUMIDITY, EP_HEATER_HOT, EP_HEATER_COLD, EP_HEXASENSE_BUTTON_STATE
#elif HAVE(TEMPSENSOR)
 #define VALUE_BROADCAST_AUTO_EIDS EP_TEMPERATURE
#elif HAVE(HUMIDITYSENSOR)
 #define VALUE_BROADCAST_AUTO_EIDS EP_TEMPERATURE, EP_HUMIDITY
#elif HAVE(PRESSURESENSOR)
 #define VALUE_BROADCAST_AUTO_EIDS EP_TEMPERATURE, EP_PRESSURE
#endif
#if !defined(VALUE_BROADCAST_AUTO_EIDS)
 #error "Don't know which EIDs to broadcast"
#endif

// relay
#define RELAY_ENABLE (HAVE(PLUG) || HAVE(PLUGPLUS))

// metering
#define METERING_POWER (HAVE(PLUGPLUS))
#define METERING_IMMEDIATE_BROADCAST 1  // immediately broadcast metering value when change is measured (you still should have the EID in the VALUE_BROADCAST_AUTO_EIDS with some reasonable timeout, so that the value is also broadcast when it reaches and stays at zero)
#define METERING_IMMEDIATE_BROADCAST_NUMBER_OF_TICKS 1 // number of ticks from the meter until a broadcast is triggered. 1: broadcast every tick ~ roughly every 2 seconds at 100W
#define METERING_IMMEDIATE_BROADCAST_MINIMUM_TIMEOUT VALUE_BROADCAST_AUTO_INTERVAL // minimum number of seconds between two broadcasts, to prevent flooding the network
#define METERING_ENERGY 0
// CAUTION: METERING_ENERGY_PERSISTENT needs external power-down detection circuit. Refer to the Wiki!
#define METERING_ENERGY_PERSISTENT 0 // Persistently store energy value (number of pulses) in EEPROM. 
#define S0_ENABLE 0 //S0 meter instead of internal meter.

//i2c master
#define I2C_ENABLE (HAVE(EMOS) || HAVE(HUMIDITYSENSOR))
#define I2C_DEBUG 0

//pressure sensor
#define PRESSURE_ENABLE (HAVE(PRESSURESENSOR))
#define PRESSURE_DEBUG 0
#define PRESSURE_OVERSAMPLING 2  //0 to 3

//humidity sensor
#define HUMIDITY_ENABLE (HAVE(EMOS) || HAVE(HUMIDITYSENSOR))
#define HUMIDITY_DEBUG 0

// epaper display and at45 driver
#define EPAPER_ENABLE (HAVE(EMOS))
#define EPAPER_DEBUG 0

//pt100 temperatur sensors for heater
#define PT100_ENABLE (HAVE(EMOS))
#define PT100_DEBUG 0

// interface app for the hexasense board
#define HEXASENSE_ENABLE (HAVE(EMOS))
#define HEXASENSE_DEBUG 0



// these things are not generally enabled. If you want to use them, add a profile or enable them locally.

// window blind shutter motor control
#define SHUTTER_ENABLE 0
#define SHUTTER_DEBUG 1
#define SHUTTER_CALIBRATE_ON_BOOT 1
#define SHUTTER_INITIAL_POSITON 1

// hexapush
#define HEXAPUSH_ENABLE 0
#define HEXAPUSH_CLICK_ENABLE 1
#define HEXAPUSH_PRESS_RELEASE_ENABLE 1
#define HEXAPUSH_PRESS_DELAY 6   // assume Press after $x/CLOCK_CONF_SECOND ms, only if Click and Press/Release are enabled
#define HEXAPUSH_CLICK_LIMIT 125 // assume click until $x/CLOCK_CONF_SECOND ms
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

//ir_receiver
#define IR_RECEIVER_ENABLE 0
#define IR_RECEIVER_DEBUG 0
#define IR_REPEAT 1 // 0 disables repeat, 1 enables repeat for buttons configured in ir_receiver.h
#define IR_RECEIVER_RAW_MODE 0

#endif // HEXBAUS_CONFIG_H
