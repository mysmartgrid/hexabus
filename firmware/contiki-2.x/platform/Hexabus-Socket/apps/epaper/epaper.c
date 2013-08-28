#include "epaper.h"
#include "epd27.h"

#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "at45.h"
#include "measurement_index.h"
#define LOG_LEVEL LOG_EMERG
#include "syslog.h"

void epaper_init(void)
{
	epd27_init();
	at45_init();
}

static int old_idx = -1;

void epaper_display_measurement(uint8_t board_temp, uint8_t mes_temp, uint8_t mes_hum)
{
	cli();
	epd27_acquire();
	at45_acquire();
	struct index_entry_t entry;
	uint8_t err;
	if ((err = index_find_entry(&entry, mes_temp, mes_hum)) != AT45_TABLE_SUCCESS) {
		syslog(LOG_EMERG, "Could not find entry for %f °C/%f %%rh", mes_temp, mes_hum);
	} else {
		epd27_wait_cog_ready();
		epd27_begin(); // power up the EPD panel
		epd27_set_temperature(board_temp); // adjust for current temperature
		int idx = entry.page_idx;
		syslog(LOG_EMERG, "page %i for %i °C/%i %%rh", idx, (int) mes_temp, (int) mes_hum);
		if (old_idx == -1) {
			epd27_clear();
		} else {
			epd27_image_at45(old_idx, EPD_compensate);
			epd27_image_at45(old_idx, EPD_white);
		}
		old_idx = idx;
		epd27_image_at45(idx, EPD_inverse);
		epd27_image_at45(idx, EPD_normal);
		epd27_end();   // power down the EPD panel
	}
	at45_release();
	epd27_release();
	sei();
}
