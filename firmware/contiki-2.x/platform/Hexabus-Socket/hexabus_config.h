/*
	Config file for hexabus

	Each app that is either optional or has config settings should have a section in this file.
	There should be an ENABLE option that enables compilation and execution of the app, a DEBUG option which enables the DEBUG define of the app. You can add more options if your app is configurable somehow.
*/
#ifndef HEXABUS_CONFIG_H
#define HEXABUS_CONFIG_H

// datetime_service
#define DATETIME_SERVICE_ENABLE 1
#define DATETIME_SERVICE_DEBUG 0

// temperature
#define TEMPERATURE_ENABLE 0
#define TEMPERATURE_DEBUG 1

#endif // HEXBAUS_CONFIG_H
