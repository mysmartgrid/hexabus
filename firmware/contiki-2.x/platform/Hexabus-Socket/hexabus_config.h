/*
	Config file for hexabus

	Each app that is either optional or has config settings should have a section in this file.
	There should be an ENABLE option that enables compilation and execution of the app, a DEBUG option which enables the DEBUG define of the app. You can add more options if your app is configurable somehow.
*/
#ifndef HEXABUS_CONFIG_H
#define HEXABUS_CONFIG_H

// udp_handler
#define UDP_HANDLER_DEBUG 1

// datetime_service
#define DATETIME_SERVICE_ENABLE 0
#define DATETIME_SERVICE_DEBUG 0

// temperature
#define TEMPERATURE_ENABLE 0
#define TEMPERATURE_DEBUG 0

// value_broadcast
#define VALUE_BROADCAST_ENABLE 0
#define VALUE_BROADCAST_DEBUG 0
#define VALUE_BROADCAST_EID 2
#define VALUE_BROADCAST_INTERVAL 60

// state_machine
#define STATE_MACHINE_ENABLE 1
#define STATE_MACHINE_DEBUG 1

#endif // HEXBAUS_CONFIG_H
