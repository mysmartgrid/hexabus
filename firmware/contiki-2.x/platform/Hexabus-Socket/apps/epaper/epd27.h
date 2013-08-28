#ifndef EPD27_H
#define EPD27_H 1

#include <stdbool.h>
#include <avr/pgmspace.h> // PROGMEM, prog_uint8_t
#include "epd27_conf.h"
#include "at45.h"

typedef enum {           // Image pixel -> Display pixel
	EPD_compensate,  // B -> W, W -> B (Current Image)
	EPD_white,       // B -> N, W -> W (Current Image)
	EPD_inverse,     // B -> N, W -> B (New Image)
	EPD_normal       // B -> B, W -> W (New Image)
} EPD_stage;

/***
 * high level API
 */
void epd27_init(void);
void epd27_acquire(void);
void epd27_release(void);
// power up and power down the EPD panel
void epd27_begin(void);
void epd27_end(void);
void epd27_set_temperature(int16_t temperature);
// clear display (anything -> white)
void epd27_clear(void);
// assuming a clear (white) screen output an image (PROGMEM data)
void epd27_image_whitescreen(const prog_uint8_t *image);
// change from old image to new image (PROGMEM data)
void epd27_image_transition(const prog_uint8_t *old_image, const prog_uint8_t *new_image);
void epd27_image_at45(uint16_t pageindex, EPD_stage stage);

/***
 * low level API
 */
// single frame refresh
void epd27_frame_fixed(uint8_t fixed_value, EPD_stage stage);
void epd27_frame_data(const prog_uint8_t *new_image, EPD_stage stage);
//TODO: Check EPD_reader implementation if beneficial
//void epd27_frame_cb(uint32_t address, EPD_reader *reader, EPD_stage stage);
// stage_time frame refresh
void epd27_frame_fixed_repeat(uint8_t fixed_value, EPD_stage stage);
void epd27_frame_data_repeat(const prog_uint8_t *new_image, EPD_stage stage);
//void epd27_frame_cb_repeat(uint32_t address, EPD_reader *reader, EPD_stage stage);
// convert temperature to compensation factor
int epd27_temperature_to_factor_10x(int temperature);
// single line display - very low-level
// also has to handle AVR progmem
void epd27_line(uint16_t line, const uint8_t *data, 
    uint8_t fixed_value, bool read_progmem, EPD_stage stage);
void epd27_image_transfersection(uint16_t startline, uint16_t endline, 
    struct at45_page_t* page, EPD_stage stage);


void epd27_wait_cog_ready(void);
#endif /* EPD27_H */

