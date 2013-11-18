#ifndef EPD27_CONF_H
#define EPD27_CONF_H 1

/**
 * Definition for the 2"7 display
 */

// output
#define EPD_PIN_EPD_CS PA5
#define EPD_PORT_EPD_CS PORTA
#define EPD_DDR_EPD_CS DDRA

// output
#define EPD_PIN_PANEL_ON PA1
#define EPD_PORT_PANEL_ON PORTA
#define EPD_DDR_PANEL_ON DDRA

// BORDER 
#define EPD_PIN_BORDER PB2
#define EPD_PORT_BORDER PORTB
#define EPD_DDR_BORDER DDRB

// output
#define EPD_PIN_DISCHARGE PB0
#define EPD_PORT_DISCHARGE PORTB
#define EPD_DDR_DISCHARGE DDRB

// output
#define EPD_PIN_PWM PB3
#define EPD_PORT_PWM PORTB
#define EPD_DDR_PWM DDRB

// output
#define EPD_PIN_RESET PA6
#define EPD_PORT_RESET PORTA
#define EPD_DDR_RESET DDRA

//input
#define EPD_PIN_BUSY PD3
#define EPD_PORT_BUSY PORTD
#define EPD_INPUT_PORT_BUSY PIND
#define EPD_DDR_BUSY DDRD

#define STAGE_TIME 630
#define LINES_PER_DISPLAY 176
#define DOTS_PER_LINE 264
#define BYTES_PER_LINE 264/8
#define BYTES_PER_SCAN 176/4
#define FILLER true

// Definitions for the AT45 memory stored screen pages.
// See ruby tools: lib/imagetools/config.rb
//#define BYTES_PER_LINE 33;
#define LINES_PER_PAGE 16
#define PAGES_PER_SCREEN 11

#endif /* EPD27_CONF_H */
