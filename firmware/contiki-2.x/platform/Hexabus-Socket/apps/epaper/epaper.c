#include "epaper.h"
#include "epd27.h"

#include <avr/pgmspace.h>

#include "cat_2_7.xbm"

void epaper_cat()
{
  epd27_wait_cog_ready();
  epd27_begin(); // power up the EPD panel
  epd27_set_temperature(22); // adjust for current temperature
  epd27_clear();
  epd27_image_whitescreen(cat_2_7_bits);
  epd27_end();   // power down the EPD panel
}
