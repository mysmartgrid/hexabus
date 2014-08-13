#include "contiki.h"
#include "hexabus_config.h"

#include "hexabus_app_bootstrap.h"

#include "button.h"
#include "relay.h"
#include "metering.h"

#include "shutter.h"
#include "hexapush.h"
#include "presence_detector.h"
#include "temperature.h"
#include "i2cmaster.h"
#include "hexonoff.h"
#include "humidity.h"
#include "analogread.h"
#include "pressure.h"
#include "ir_receiver.h"
#include "sm_upload.h"
#include "pt100.h"
#include "value_broadcast.h"
#include "datetime_service.h"
#include "memory_debugger.h"
#include "epaper.h"
#include "hexasense.h"
#include "udp_handler.h"
#include "state_machine.h"

#if WEBSERVER
#include "httpd-fs.h"
#include "httpd-cgi.h"
#include "webserver-nogui.h"
#endif

#if HEXABUS_BOOTSTRAP_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void hexabus_bootstrap_init_apps()
{
	PRINTF("Initializing all EIDs and apps...\n");

#if MEMORY_DEBUGGER_ENABLE
	memory_debugger_init();
#endif

  /* Process for periodic sending of HEXABUS data */
#if VALUE_BROADCAST_ENABLE && !VALUE_BROADCAST_AUTO_INTERVAL
	init_value_broadcast();
#endif

#if STATE_MACHINE_ENABLE
	sm_start();
#endif

	relay_init();

	metering_stop();
	metering_init();
	metering_start();

#if I2C_ENABLE
	i2c_init();
#endif

#if SHUTTER_ENABLE
	shutter_init();
	process_start(&shutter_setup_process, NULL);
#endif
#if HUMIDITY_ENABLE
	humidity_init();
#endif
#if PRESENCE_DETECTOR_ENABLE
	presence_detector_init();
#endif
#if HEXAPUSH_ENABLE
	hexapush_init();
#endif
#if TEMPERATURE_ENABLE
	temperature_init();
#endif
#if HEXONOFF_ENABLE
	hexonoff_init();
#endif
#if ANALOGREAD_ENABLE
	analogread_init();
#endif
#if PRESSURE_ENABLE
	pressure_init();
#endif
#if IR_RECEIVER_ENABLE
	ir_receiver_init();
#endif
#if SM_UPLOAD_ENABLE
	sm_upload_init();
#endif
#if PT100_ENABLE
	pt100_init();
#endif
#if EPAPER_ENABLE
	epaper_init();
	epaper_display_special(EP_SCREEN_BOOT);
#endif
#if HEXASENSE_ENABLE
	hexasense_init();
#endif
}

void hexabus_bootstrap_start_processes()
{
	PRINTF("Starting processes...\n");

#if MEMORY_DEBUGGER_ENABLE
	process_start(&memory_debugger_process, NULL);
#endif

	process_start(&udp_handler_process, NULL);

  /* Process for periodic sending of HEXABUS data */
#if VALUE_BROADCAST_ENABLE && VALUE_BROADCAST_AUTO_INTERVAL
	process_start(&value_broadcast_process, NULL);
#endif

#if DATETIME_SERVICE_ENABLE
	process_start(&datetime_service_process, NULL);
#endif

	process_start(&button_pressed_process, NULL);

#if SHUTTER_ENABLE
	process_start(&shutter_process, NULL);
#endif
#if PRESSURE_ENABLE
	process_start(&pressure_process, NULL);
#endif
#if IR_RECEIVER_ENABLE
	process_start(&ir_receiver_process, NULL);
#endif
#if WEBSERVER
	process_start(&webserver_nogui_process, NULL);
#endif
}
