#include "hexabus_config.h"

#include "epaper.h"
#include "epd27.h"

#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include <sys/clock.h>

#include "at45.h"
#include "measurement_index.h"
#define LOG_LEVEL EPAPER_DEBUG
#include "syslog.h"

#include "../../../../shared/endpoints.h"

void epaper_init(void)
{
	epd27_init();
	at45_init();
}


static int16_t find_page_index(uint8_t temp, uint8_t hum)
{
	struct index_entry_t entry;
	if (index_find_entry(&entry, temp, hum) != AT45_TABLE_SUCCESS) {
		return -1;
	} else {
		return entry.page_idx;
	}
}

static void acquire(void)
{
	cli();
	epd27_acquire();
	at45_acquire();
}

static void release(void)
{
	at45_release();
	epd27_release();
	sei();
}

static int16_t old_idx = -1;

static void display_page(uint8_t board_temp, uint16_t idx)
{
	epd27_wait_cog_ready();
	epd27_begin(); // power up the EPD panel
	epd27_set_temperature(board_temp); // adjust for current temperature
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

void epaper_display_measurement(uint8_t board_temp, uint8_t mes_temp, uint8_t mes_hum)
{
	acquire();
	int16_t idx = find_page_index(mes_temp, mes_hum);
	if (idx < 0) {
		syslog(LOG_EMERG, "Could not find entry for %i °C/%i %%rh", mes_temp, mes_hum);
	} else {
		syslog(LOG_DEBUG, "page %i for %i °C/%i %%rh", idx, (int) mes_temp, (int) mes_hum);
		display_page(board_temp, idx);
	}
	release();
}


void epaper_update_measurement(uint8_t board_temp, uint8_t mes_temp, uint8_t mes_hum)
{
	acquire();
	
	static int16_t skipped_update_idx = -1;
	static uint32_t last_update_at = 0;

	// clamp temperature to first out-of-range values, as those are rendered
	// as "<X" and ">Y", respectively
	if (mes_temp < 15) {
		mes_temp = 14;
	} else if (mes_temp > 30) {
		mes_temp = 31;
	}
	// clamp temperature to valid range
	if (mes_hum < 10) {
		mes_hum = 10;
	} else if (mes_hum > 90) {
		mes_hum = 90;
	}

	uint32_t now = clock_seconds();
	uint32_t diff = last_update_at < now
		? now - last_update_at
		: now + 1 + (UINT32_MAX - last_update_at);
	int16_t screen_idx = find_page_index(mes_temp, mes_hum);

	syslog(LOG_DEBUG, "Updating display to %i °C/%i %%rh", mes_temp, mes_hum);
	if (screen_idx < 0) {
		syslog(LOG_EMERG, "Could not find entry for %i °C/%i %%rh", mes_temp, mes_hum);
		goto end;
	}

	if (old_idx != screen_idx 
			&& ((diff >= 10*60)
				|| (last_update_at == 0)
				|| (skipped_update_idx != -1 && skipped_update_idx != screen_idx))) {
		last_update_at = now;
		skipped_update_idx = -1;
		syslog(LOG_DEBUG, "Redrawing display");
		display_page(board_temp, screen_idx);
	} else if (skipped_update_idx == -1 && old_idx != screen_idx) {
		syslog(LOG_DEBUG, "Not redrawing display yet [1]");
		skipped_update_idx = screen_idx;
	} else {
		syslog(LOG_DEBUG, "No reason to redraw display");
	}

end:
	release();
}

static uint8_t current_temperature = 0xff;
static uint8_t current_humidity = 0xff;

void epaper_handle_input(struct hxb_value* val, uint32_t eid)
{
	if (eid == EP_TEMPERATURE) {
		current_temperature = val->v_float;
	} else if (eid == EP_HUMIDITY) {
		current_humidity = val->v_float;
	} else {
		return;
	}

	if (current_temperature != 0xff && current_humidity != 0xff) {
		epaper_update_measurement(current_temperature,
				current_temperature,
				10 * (current_humidity / 10));
	} else {
		syslog(LOG_DEBUG, "Not updating display, not ready (%u %u)", current_temperature, current_humidity);
	}
}

void epaper_display_special(enum epaper_special_screen screen)
{
	acquire();

	uint16_t page_idx = index_get_special_screen(screen);
	if (!page_idx) {
		syslog(LOG_ERR, "Could not dislay special screen %i", screen);
		goto end;
	}

	display_page(20, page_idx);

end:
	release();
}
