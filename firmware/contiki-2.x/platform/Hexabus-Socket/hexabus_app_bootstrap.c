#include "contiki.h"
#include "hexabus_config.h"

#include "hexabus_app_bootstrap.h"

#include "button.h"
#include "relay.h"
#include "metering.h"

#if SHUTTER_ENABLE
#include "shutter.h"
#endif
#if HEXAPUSH_ENABLE
#include "hexapush.h"
#endif
#if PRESENCE_DETECTOR_ENABLE
#include "presence_detector.h"
#endif
#if TEMPERATURE_ENABLE
#include "temperature.h"
#endif
#if I2C_ENABLE
#include "i2cmaster.h"
#endif
#if HEXONOFF_ENABLE
#include "hexonoff.h"
#endif
#if ANALOGREAD_ENABLE
#include "analogread.h"
#endif
#if PRESSURE_ENABLE
#include "pressure.h"
#endif
#if IR_RECEIVER_ENABLE
#include "ir_receiver.h"
#endif

#if HEXABUS_BOOTSTRAP_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void hexabus_bootstrap_init_apps() {

    PRINTF("Initializing all EIDs...\n");
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

}

void hexabus_bootstrap_start_processes() {

    PRINTF("Starting processes...\n");

    process_start(&button_pressed_process, NULL);

#if SHUTTER_ENABLE
    process_start(&shutter_process, NULL);
#endif
#if HEXAPUSH_ENABLE
    process_start(&hexapush_process, NULL);
#endif
#if PRESSURE_ENABLE
    process_start(&pressure_process, NULL);
#endif
#if IR_RECEIVER_ENABLE
    process_start(&ir_receiver_process, NULL);
#endif

}
