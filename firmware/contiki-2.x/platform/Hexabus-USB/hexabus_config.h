/*
Config file for hexabus

Each app that is either optional or has config settings should have a section in this file.
There should be an ENABLE option that enables compilation and execution of the app, a DEBUG option which enables the DEBUG define of the app. You can add more options if your app is configurable somehow.
*/
#ifndef HEXABUS_CONFIG_H
#define HEXABUS_CONFIG_H

// button
#define BUTTON_PIN		PINE
#define BUTTON_BIT		PE2

#define BUTTON_DEBOUNCE_TICKS		1
#define	BUTTON_CLICK_MS					2000UL
#define	BUTTON_LONG_CLICK_MS		7000UL

#endif // HEXBAUS_CONFIG_H
