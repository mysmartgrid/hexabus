/*
	Config file for hexabus

	Each app that is either optional or has config settings should have a section in this file.
	There should be an ENABLE option that enables compilation and execution of the app, a DEBUG option which enables the DEBUG define of the app. You can add more options if your app is configurable somehow.
*/
#ifndef HEXABUS_CONFIG_H
#define HEXABUS_CONFIG_H

// udp_handler
#define UDP_HANDLER_DEBUG 0

// datetime_service
#define DATETIME_SERVICE_ENABLE 0
#define DATETIME_SERVICE_DEBUG 0

// temperature
#define TEMPERATURE_ENABLE 0
#define TEMPERATURE_DEBUG 0

// value_broadcast
#define VALUE_BROADCAST_ENABLE 1
#define VALUE_BROADCAST_DEBUG 0
#define VALUE_BROADCAST_AUTO_EID 24
#define VALUE_BROADCAST_AUTO_INTERVAL 0 //0 to disable automatic broadcast

// state_machine
#define STATE_MACHINE_ENABLE 0
#define STATE_MACHINE_DEBUG 0

// shutter
#define SHUTTER_ENABLE 0
#define SHUTTER_DEBUG 0

// hexapush
#define HEXAPUSH_ENABLE 1
#define HEXAPUSH_CLICK_ENABLE 0
#define HEXAPUSH_PRESS_RELEASE_ENABLE 1
#define HEXAPUSH_PRESS_DELAY 6   //multiplied by 50ms, only if Click and Press/Release are enabled
#define HEXAPUSH_DEBUG 0

// presence detector
#define PRESENCE_DETECTOR_ENABLE 0
#define PRESENCE_DETECTOR_DEBUG 0

// hexonoff
#define HEXONOFF_ENABLE 1
#define HEXONOFF_DEBUG 1
#define INITIAL_VALUE 12

#endif // HEXBAUS_CONFIG_H
