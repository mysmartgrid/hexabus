#include "epaper.h"
#include "epd27.h"

#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "at45.h"

#include "cat_2_7.xbm"

static int idx = 1;
static int old_idx = -1;

void epaper_cat()
{
	cli();
	epd27_acquire();
	at45_acquire();
  epd27_wait_cog_ready();
  epd27_begin(); // power up the EPD panel
  epd27_set_temperature(22); // adjust for current temperature
	if (old_idx == -1) {
		epd27_clear();
	} else {
		epd27_image_at45(4 + 11 * old_idx, EPD_compensate);
		epd27_image_at45(4 + 11 * old_idx, EPD_white);
	}
	old_idx = idx;
	epd27_image_at45(4 + 11 * idx, EPD_inverse);
	epd27_image_at45(4 + 11 * idx, EPD_normal);
	if (++idx == 6)
		idx = 0;
//  epd27_image_whitescreen(cat_2_7_bits);
  epd27_end();   // power down the EPD panel
	at45_release();
	epd27_release();
	sei();
}
