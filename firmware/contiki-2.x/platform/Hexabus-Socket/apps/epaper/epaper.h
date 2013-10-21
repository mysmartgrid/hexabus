#ifndef APPS_EPAPER_EPAPER_H
#define APPS_EPAPER_EPAPER_H 1

#include <stdint.h>
#include "hexabus_packet.h"

enum epaper_special_screen {
	EP_SCREEN_BOOT = 0,
	EP_SCREEN_HARDWARE_ERROR = 1,
	EP_SCREEN_NO_CONNECTION = 2
};

void epaper_init(void);

void epaper_display_special(enum epaper_special_screen screen);

// display a measurement tuple of (temperature, humidity) [°C, %r.h.] on the epaper display
// images are pulled from an associated AT45 flash chip, the data layout is assumed to be
// as specified by the hexasense flash generator
// NOTE: this function assumes complete control of the device. all interrupts will be disabled
// and GPIO ports will be reconfigured as necessary for the operation. The watchdog will be
// kept alive during the operation, and all modifications will be reverted upon exit.
// NOTE: this functions may take a long time to run
// NOTE: this function will not clamp measurement values
void epaper_display_measurement(uint8_t board_temp, uint8_t mes_temp, uint8_t mes_hum);

// this function behaves much like epaper_display_measurement, but updates the screen only if
//   a) the last display update was more than ten minutes ago, or
//   b) past updates since a full refresh would have caused a display change already
// measurements are clamped to the range [14,31] °C and [10,90] %rh
void epaper_update_measurement(uint8_t board_temp, uint8_t mes_temp, uint8_t mes_hum);

// forwards values from EP_TEMPERATURE and EP_HUMIDITY to epaper_update_measurement
void epaper_handle_input(struct hxb_value* val, uint32_t eid);

#endif
