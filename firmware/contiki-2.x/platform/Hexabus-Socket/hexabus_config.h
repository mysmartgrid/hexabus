/*
	Config file for hexabus

	Each app that is either optional or has config settings should have a section in this file.
	There should be an ENABLE option that enables compilation and execution of the app, a DEBUG option which enables the DEBUG define of the app. You can add more options if your app is configurable somehow.
*/
#ifndef HEXABUS_CONFIG_H
#define HEXABUS_CONFIG_H

// udp_handler
#define UDP_HANDLER_DEBUG 0

// WebServer
#define WEBSERVER_DEBUG 0

// button
#define BUTTON_DEBUG 0
#define BUTTON_DOUBLE_CLICK_ENABLED 0
#define BUTTON_HAS_EID 0 // set to 1 to have button on EID 4. Set to 0 to have button not do any interaction with network
#define BUTTON_TOGGLES_RELAY 1 // set to 1 to have the button toggle the relay directly

// datetime_service
#define DATETIME_SERVICE_ENABLE 1
#define DATETIME_SERVICE_DEBUG 0

// temperature
#define TEMPERATURE_ENABLE 0
#define TEMPERATURE_DEBUG 0

// value_broadcast
#define VALUE_BROADCAST_ENABLE 1
#define VALUE_BROADCAST_DEBUG 1
#define VALUE_BROADCAST_NUMBER_OF_AUTO_EIDS 2
#define VALUE_BROADCAST_AUTO_EIDS 2, 4
#define VALUE_BROADCAST_AUTO_INTERVAL 20 //0 to disable automatic broadcast
#define VALUE_BROADCAST_NUMBER_OF_LOCAL_ONLY_EIDS 0
#define VALUE_BROADCAST_LOCAL_ONLY_EIDS 2

// state_machine
#define STATE_MACHINE_ENABLE 1
#define STATE_MACHINE_DEBUG 0

// shutter
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
#define PRESENCE_DETECTOR_SERVER_TIMEOUT 3 //in Minutes
#define PRESENCE_DETECTOR_CLIENT_GROUP 2 // 2 to 255
#define PRESENCE_DETECTOR_CLIENT_KEEP_ALIVE 60 // in seconds, 0 to disable

// hexonoff
#define HEXONOFF_ENABLE 0
#define HEXONOFF_DEBUG 0
#define HEXONOFF_INITIAL_VALUE 0

// lightsensor
#define ANALOGREAD_ENABLE 0
#define ANALOGREAD_DEBUG 1
#define ANALOGREAD_PIN 0 // 0 to 7
#define ANALOGREAD_MULT 0.0024414062 //  0.0024414062 for volts at 2.5V supply voltage
#define ANALOGREAD_EID 29

#endif // HEXBAUS_CONFIG_H
