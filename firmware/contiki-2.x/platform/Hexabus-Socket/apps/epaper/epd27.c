#include "epd27.h"
#include "_spi.h"
#include <stdint.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <dev/watchdog.h>

#define uart_puts_P printf

// inline arrays
#define ARRAY(type, ...) ((type[]){__VA_ARGS__})
#define CU8(...) (ARRAY(const uint8_t, __VA_ARGS__))


// STAGE_TIME is smaller than the time needed for one iteration at the default
// frequency of a hexabus device with a temperature correction factor for 22°C
// this, store only an iteration count relative to that.
static uint16_t epd27_stage_iterations = 0;
// magic foo.
static uint8_t channel_select[] = {0x72, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfe, 0x00, 0x00};
static uint8_t gate_source[] = {0x72, 0x00};
uint16_t channel_select_length = sizeof(channel_select);
uint16_t gate_source_length = sizeof(gate_source);

/**
 * helper functions
 */

// map to spi library
void SPI_put(uint8_t c) {
  spi_rw(c);
}

void epd27_wait_cog_ready(void) {
	while ((1 << EPD_PIN_BUSY) == 
      (EPD_INPUT_PORT_BUSY & (1<<EPD_PIN_BUSY))) {
    ;;
	}
}

void epd27_cs_low(void) {
  EPD_PORT_EPD_CS &= ~(1<<EPD_PIN_EPD_CS);
}

void epd27_cs_high(void) {
  EPD_PORT_EPD_CS |= (1<<EPD_PIN_EPD_CS);
}

void SPI_put_wait(uint8_t c) {
	SPI_put(c);
  epd27_wait_cog_ready();
}

void SPI_send(const uint8_t *buffer, uint16_t length) {
	// CS low
  epd27_cs_low();
	// send all data
	for (uint16_t i = 0; i < length; ++i) {
		SPI_put(*buffer++);
	}
	// CS high
  epd27_cs_high();
}


static void PWM_start() {
  TCCR0B |= (1<<CS10); // enable counter, no prescaling
}


static void PWM_stop() {
  TCCR0B &= ~((1<<CS10)); // disable counter
  //_delay_ms(1);
  EPD_PORT_PWM &= ~(1 << EPD_PIN_PWM); // set pin low.
}


/**
 * high level API
 */

static uint8_t port_pwm, ddr_pwm, tccr0a, ocr0b;

static void epd27_pwm_acquire(void) {
	port_pwm = EPD_PORT_PWM;
	ddr_pwm = EPD_DDR_PWM;
	tccr0a = TCCR0A;
	ocr0b = OCR0B;
  // set output, initial value low
  EPD_PORT_PWM |= (1 << EPD_PIN_PWM);
  EPD_DDR_PWM |= (1 << EPD_PIN_PWM);
  EPD_PORT_PWM &= ~(1 << EPD_PIN_PWM);
  // PWM is PD5, which is OC0B. Use hardware PWM, see 
  // http://www.mikrocontroller.net/articles/AVR_PWM
  //DDRB |= (1<<OCR0B); // Port OC1B mit angeschlossener LED als Ausgang
  TCCR0A = (1<<WGM11) | (1<<WGM10) | (1<<COM0B1); // PWM, phase correct, 8 bit.
  OCR0B = 128-1; // Duty cycle 50% (Anm. ob 128 oder 127 bitte prüfen) 
}

static void epd27_pwm_release()
{
	EPD_PORT_PWM = port_pwm;
	EPD_DDR_PWM = ddr_pwm;
	TCCR0A = tccr0a;
	OCR0B = ocr0b;
}


void epd27_init()
{
	// Configure output pins.
	EPD_PORT_EPD_CS |= (1 << EPD_PIN_EPD_CS);
	EPD_DDR_EPD_CS |= (1 << EPD_PIN_EPD_CS);
	EPD_PORT_PANEL_ON |= (1 << EPD_PIN_PANEL_ON);
	EPD_DDR_PANEL_ON |= (1 << EPD_PIN_PANEL_ON);
	//EPD_PORT_BORDER |= (1 << EPD_PIN_BORDER);
	//EPD_DDR_BORDER |= (1 << EPD_PIN_BORDER);
	EPD_PORT_DISCHARGE |= (1 << EPD_PIN_DISCHARGE);
	EPD_DDR_DISCHARGE |= (1 << EPD_PIN_DISCHARGE);
	EPD_PORT_RESET |= (1 << EPD_PIN_RESET);
	EPD_DDR_RESET |= (1 << EPD_PIN_RESET);

	// Configure input pins.
	EPD_PORT_BUSY &= ~(1 << EPD_PIN_BUSY); 
	EPD_DDR_BUSY &= ~(1 << EPD_PIN_BUSY); 

	// set initial values - all low.
	EPD_PORT_RESET &= ~(1 << EPD_PIN_RESET);
	EPD_PORT_PANEL_ON &= ~(1 << EPD_PIN_PANEL_ON);
	//EPD_PORT_BORDER &= ~(1 << EPD_PIN_BORDER);
	EPD_PORT_DISCHARGE &= ~(1 << EPD_PIN_DISCHARGE);
	//EPD_PORT_EPD_CS &= ~(1 << EPD_PIN_EPD_CS);
}

void epd27_acquire(void) {
  uart_puts_P("\r\nEPD27 init.");

  epd27_cs_low();

  epd27_pwm_acquire();
}

void epd27_release()
{
	epd27_pwm_release();
}


void epd27_image_transfersection(uint16_t startline, uint16_t endline, 
    struct at45_page_t* page, EPD_stage stage) {
  uint8_t line_data[BYTES_PER_LINE];
 // long stage_time = epd27_get_factored_stage_time();
 // do {
 //   uint32_t t_start = millis();
    for (uint16_t line = startline; line <= endline ; ++line) {
      memcpy(&line_data, &page->data[((line - startline)*BYTES_PER_LINE)], BYTES_PER_LINE);
      epd27_line(line, line_data, 0, false, stage);
    }
 //   uint32_t t_end = millis();
 //   if (t_end > t_start) {
 //     stage_time -= t_end - t_start;
 //   } else {
 //     stage_time -= t_start - t_end + 1 + UINT32_MAX;
 //   }
 // } while (stage_time > 0);


}

void epd27_image_at45(uint16_t pageindex, EPD_stage stage) {
  struct at45_page_t* page = malloc(sizeof(uint8_t [AT45_PAGE_SIZE]));
  // for all pages:
  for( uint16_t curpage_idx = pageindex; 
      curpage_idx < (pageindex + PAGES_PER_SCREEN); curpage_idx+= 1) {
    // 1. get from at45
    memset(page, 0x00, AT45_PAGE_SIZE);
    if (! at45_read_page(page, curpage_idx)) {
      free(page);
      uart_puts_P("Failed to read page from AT45 - ABORTING");
      return;
    } else {
      //uart_puts_P("\r\nPage ");
      //char conversion_buffer[50];
      //itoa(curpage_idx, conversion_buffer, 10);
      //uart_puts(conversion_buffer);
      //uart_puts_P(": ");
      //at45_dump_page(page);
      // 2. loop:
      // calculate the section of the screen we're going to fill using this page
      uint16_t startline = (curpage_idx - pageindex) * LINES_PER_PAGE;
      uint16_t endline = (curpage_idx - pageindex + 1) * LINES_PER_PAGE - 1;
      //uart_puts_P(" Startline: ");
      //itoa(startline, conversion_buffer, 10);
      //uart_puts(conversion_buffer);
      //uart_puts_P(" Endline: ");
      //itoa(endline, conversion_buffer, 10);
      //uart_puts(conversion_buffer);
      epd27_image_transfersection(startline, endline, page, stage);
    }
  }
  free(page);
}



static void spi_delay10us_send(const uint8_t *buffer, uint16_t length)
{
	_delay_us(10);
	SPI_send(buffer, length);
}

void epd27_begin(void) {
  // power up sequence
  uart_puts_P("\r\nEPD27 begin.");
	SPI_put(0x00);
  // set initial values - all low.
  EPD_PORT_RESET &= ~(1 << EPD_PIN_RESET);
  EPD_PORT_PANEL_ON &= ~(1 << EPD_PIN_PANEL_ON);
  //EPD_PORT_BORDER &= ~(1 << EPD_PIN_BORDER);
  EPD_PORT_DISCHARGE &= ~(1 << EPD_PIN_DISCHARGE);
  //EPD_PORT_EPD_CS &= ~(1 << EPD_PIN_EPD_CS);
  epd27_cs_low();

	PWM_start();
	_delay_ms(5);
	//digitalWrite(this->EPD_Pin_PANEL_ON, HIGH);
  EPD_PORT_PANEL_ON |= (1 << EPD_PIN_PANEL_ON);
	_delay_ms(10);

	//digitalWrite(this->EPD_Pin_RESET, HIGH);
  EPD_PORT_RESET |= (1 << EPD_PIN_RESET);
	//digitalWrite(this->EPD_Pin_BORDER, HIGH);
  // IGNORED
	//digitalWrite(this->EPD_Pin_EPD_CS, HIGH);
  //EPD_PORT_EPD_CS |= (1 << EPD_PIN_EPD_CS);
  epd27_cs_high();
	_delay_ms(5);

	//digitalWrite(this->EPD_Pin_RESET, LOW);
  EPD_PORT_RESET &= ~(1 << EPD_PIN_RESET);
	_delay_ms(5);

	//digitalWrite(this->EPD_Pin_RESET, HIGH);
  EPD_PORT_RESET |= (1 << EPD_PIN_RESET);
	_delay_ms(5);

	// wait for COG to become ready
  epd27_wait_cog_ready();

// channel select
	spi_delay10us_send(CU8(0x70, 0x01), 2);
	spi_delay10us_send(channel_select, channel_select_length);

	// DC/DC frequency
	spi_delay10us_send(CU8(0x70, 0x06), 2);
	spi_delay10us_send(CU8(0x72, 0xff), 2);

	// high power mode osc
	spi_delay10us_send(CU8(0x70, 0x07), 2);
	spi_delay10us_send(CU8(0x72, 0x9d), 2);


	// disable ADC
	spi_delay10us_send(CU8(0x70, 0x08), 2);
	spi_delay10us_send(CU8(0x72, 0x00), 2);

	// Vcom level
	spi_delay10us_send(CU8(0x70, 0x09), 2);
	spi_delay10us_send(CU8(0x72, 0xd0, 0x00), 3);

	// gate and source voltage levels
	spi_delay10us_send(CU8(0x70, 0x04), 2);
	spi_delay10us_send(gate_source, gate_source_length);

	_delay_ms(5);  //???

	// driver latch on
	spi_delay10us_send(CU8(0x70, 0x03), 2);
  spi_delay10us_send(CU8(0x72, 0x01), 2);

	// driver latch off
	spi_delay10us_send(CU8(0x70, 0x03), 2);
	spi_delay10us_send(CU8(0x72, 0x00), 2);

	_delay_ms(5);

	// charge pump positive voltage on
	spi_delay10us_send(CU8(0x70, 0x05), 2);
	spi_delay10us_send(CU8(0x72, 0x01), 2);

	// final delay before PWM off
	_delay_ms(30);
	PWM_stop();

	// charge pump negative voltage on
	spi_delay10us_send(CU8(0x70, 0x05), 2);
	spi_delay10us_send(CU8(0x72, 0x03), 2);

	_delay_ms(30);

	// Vcom driver on
	spi_delay10us_send(CU8(0x70, 0x05), 2);
	spi_delay10us_send(CU8(0x72, 0x0f), 2);

	_delay_ms(30);

	// output enable to disable
	spi_delay10us_send(CU8(0x70, 0x02), 2);
	spi_delay10us_send(CU8(0x72, 0x24), 2);
}

void epd27_end(void) {
  uart_puts_P("\r\nEPD27 end.");
  epd27_frame_fixed(0x55, EPD_normal); // dummy frame
	epd27_line(0x7fffu, 0, 0x55, false, EPD_normal); // dummy_line
	_delay_ms(25);
  // IGNORE
	//digitalWrite(this->EPD_Pin_BORDER, LOW);
	//_delay_ms(30);
	//digitalWrite(this->EPD_Pin_BORDER, HIGH);

	// latch reset turn on
	spi_delay10us_send(CU8(0x70, 0x03), 2);
	spi_delay10us_send(CU8(0x72, 0x01), 2);

	// output enable off
	spi_delay10us_send(CU8(0x70, 0x02), 2);
	spi_delay10us_send(CU8(0x72, 0x05), 2);

	// Vcom power off
	spi_delay10us_send(CU8(0x70, 0x05), 2);
	spi_delay10us_send(CU8(0x72, 0x0e), 2);

	// power off negative charge pump
	spi_delay10us_send(CU8(0x70, 0x05), 2);
	spi_delay10us_send(CU8(0x72, 0x02), 2);

	// discharge
	spi_delay10us_send(CU8(0x70, 0x04), 2);
	spi_delay10us_send(CU8(0x72, 0x0c), 2);

	_delay_ms(120);

	// all charge pumps off
	spi_delay10us_send(CU8(0x70, 0x05), 2);
	spi_delay10us_send(CU8(0x72, 0x00), 2);

	// turn of osc
	spi_delay10us_send(CU8(0x70, 0x07), 2);
	spi_delay10us_send(CU8(0x72, 0x0d), 2);

	// discharge internal - 1
	spi_delay10us_send(CU8(0x70, 0x04), 2);
	spi_delay10us_send(CU8(0x72, 0x50), 2);

	_delay_ms(40);

	// discharge internal - 2
	spi_delay10us_send(CU8(0x70, 0x04), 2);
	spi_delay10us_send(CU8(0x72, 0xA0), 2);

	_delay_ms(40);

	// discharge internal - 3
	spi_delay10us_send(CU8(0x70, 0x04), 2);
	spi_delay10us_send(CU8(0x72, 0x00), 2);

	// turn of power and all signals
	//digitalWrite(this->EPD_Pin_RESET, LOW);
  EPD_PORT_RESET &= ~(1 << EPD_PIN_RESET);
  // CONTINUE BELOW!
	//digitalWrite(this->EPD_Pin_PANEL_ON, LOW);
  EPD_PORT_PANEL_ON &= ~(1 << EPD_PIN_PANEL_ON);
	//digitalWrite(this->EPD_Pin_BORDER, LOW);
	//digitalWrite(this->EPD_Pin_EPD_CS, LOW);
  //EPD_PORT_EPD_CS &= ~(1 << EPD_PIN_EPD_CS);
  epd27_cs_low();
	//digitalWrite(this->EPD_Pin_DISCHARGE, HIGH);
  EPD_PORT_DISCHARGE |= (1 << EPD_PIN_DISCHARGE);

	SPI_put(0x00);
	_delay_ms(150);
	//digitalWrite(this->EPD_Pin_DISCHARGE, LOW);
  EPD_PORT_DISCHARGE &= ~(1 << EPD_PIN_DISCHARGE);

}

void  epd27_set_temperature(int16_t temperature) {
	epd27_stage_iterations = epd27_temperature_to_factor_10x(temperature) / epd27_temperature_to_factor_10x(22);
	if (epd27_stage_iterations == 0)
		epd27_stage_iterations = 1;
}

void epd27_clear() {
  uart_puts_P("EPD27 clear.");
  epd27_frame_fixed_repeat(0xff, EPD_compensate);
  epd27_frame_fixed_repeat(0xff, EPD_white);
  epd27_frame_fixed_repeat(0xaa, EPD_inverse);
  epd27_frame_fixed_repeat(0xaa, EPD_normal);
}

void epd27_image_whitescreen(const prog_uint8_t *image) {
  epd27_frame_fixed_repeat(0xaa, EPD_compensate);
  epd27_frame_fixed_repeat(0xaa, EPD_white);
  epd27_frame_data_repeat(image, EPD_inverse);
  epd27_frame_data_repeat(image, EPD_normal);
}

void epd27_image_transition(const prog_uint8_t *old_image, const prog_uint8_t *new_image) {
  epd27_frame_data_repeat(old_image, EPD_compensate);
  epd27_frame_data_repeat(old_image, EPD_white);
  epd27_frame_data_repeat(new_image, EPD_inverse);
  epd27_frame_data_repeat(new_image, EPD_normal);
}

/***
 * low level API
 */
// single frame refresh
void epd27_frame_fixed(uint8_t fixed_value, EPD_stage stage) {
  for (uint8_t line = 0; line < LINES_PER_DISPLAY ; ++line) {
		epd27_line(line, 0, fixed_value, false, stage);
	}
}

void epd27_frame_data(const prog_uint8_t *image, EPD_stage stage) {
  for (uint8_t line = 0; line < LINES_PER_DISPLAY ; ++line) {
		epd27_line(line, &image[line * BYTES_PER_LINE], 0, true, stage);
	}
}


void epd27_frame_fixed_repeat(uint8_t fixed_value, EPD_stage stage)
{
  uart_puts_P("\r\nEPD27 frame_fixed_repeat");
	for (int16_t iterations_left = epd27_stage_iterations; iterations_left > 0; iterations_left--) {
    uart_puts_P(".");
		epd27_frame_fixed(fixed_value, stage);
	}
}

void epd27_frame_data_repeat(const prog_uint8_t *new_image, EPD_stage stage){
	for (int16_t iterations_left = epd27_stage_iterations; iterations_left > 0; iterations_left--) {
		epd27_frame_data(new_image, stage);
	}
}

int epd27_temperature_to_factor_10x(int temperature) {
  if (temperature <= -10) {
    return 170;
  } else if (temperature <= -5) {
    return 120;
  } else if (temperature <= 5) {
    return 80;
  } else if (temperature <= 10) {
    return 40;
  } else if (temperature <= 15) {
    return 30;
  } else if (temperature <= 20) {
    return 20;
  } else if (temperature <= 40) {
    return 10;
  }
  return 7;

}

// single line display - very low-level
// also has to handle AVR progmem
void epd27_line(uint16_t line, const uint8_t *data, 
    uint8_t fixed_value, bool read_progmem, EPD_stage stage) {
  // charge pump voltage levels
  spi_delay10us_send(CU8(0x70, 0x04), 2);
  spi_delay10us_send(gate_source, gate_source_length);

	watchdog_periodic();

  // send data
  spi_delay10us_send(CU8(0x70, 0x0a), 2);
  _delay_us(10);

  // CS low
  //digitalWrite(this->EPD_Pin_EPD_CS, LOW);
  epd27_cs_low();
  SPI_put_wait(0x72);

  // even pixels
  for (uint16_t b = BYTES_PER_LINE; b > 0; --b) {
    if (0 != data) {
      // AVR has multiple memory spaces
      uint8_t pixels;
      if (read_progmem) {
        pixels = pgm_read_byte_near(data + b - 1) & 0xaa;
      } else {
        pixels = data[b - 1] & 0xaa;
      }
      switch(stage) {
        case EPD_compensate:  // B -> W, W -> B (Current Image)
          pixels = 0xaa | ((pixels ^ 0xaa) >> 1);
          break;
        case EPD_white:       // B -> N, W -> W (Current Image)
          pixels = 0x55 + ((pixels ^ 0xaa) >> 1);
          break;
        case EPD_inverse:     // B -> N, W -> B (New Image)
          pixels = 0x55 | (pixels ^ 0xaa);
          break;
        case EPD_normal:       // B -> B, W -> W (New Image)
          pixels = 0xaa | (pixels >> 1);
          break;
      }
      SPI_put_wait(pixels);
    } else {
      SPI_put_wait(fixed_value);
    }	
  }

  // scan line
  for (uint16_t b = 0; b < BYTES_PER_SCAN; ++b) {
    if (line / 4 == b) {
      SPI_put_wait(0xc0 >> (2 * (line & 0x03)));
    } else {
      SPI_put_wait(0x00);
    }
  }

  // odd pixels
  for (uint16_t b = 0; b < BYTES_PER_LINE; ++b) {
    if (0 != data) {
      // AVR has multiple memory spaces
      uint8_t pixels;
      if (read_progmem) {
        pixels = pgm_read_byte_near(data + b) & 0x55;
      } else {
        pixels = data[b] & 0x55;
      }
      switch(stage) {
        case EPD_compensate:  // B -> W, W -> B (Current Image)
          pixels = 0xaa | (pixels ^ 0x55);
          break;
        case EPD_white:       // B -> N, W -> W (Current Image)
          pixels = 0x55 + (pixels ^ 0x55);
          break;
        case EPD_inverse:     // B -> N, W -> B (New Image)
          pixels = 0x55 | ((pixels ^ 0x55) << 1);
          break;
        case EPD_normal:       // B -> B, W -> W (New Image)
          pixels = 0xaa | pixels;
          break;
      }
      uint8_t p1 = (pixels >> 6) & 0x03;
      uint8_t p2 = (pixels >> 4) & 0x03;
      uint8_t p3 = (pixels >> 2) & 0x03;
      uint8_t p4 = (pixels >> 0) & 0x03;
      pixels = (p1 << 0) | (p2 << 2) | (p3 << 4) | (p4 << 6);
      SPI_put_wait(pixels);
    } else {
      SPI_put_wait(fixed_value);
    }
  }

  if (FILLER) {
    SPI_put_wait(0x00);
  }

  // CS high
  //digitalWrite(this->EPD_Pin_EPD_CS, HIGH);
  epd27_cs_high();

  // output data to panel
  spi_delay10us_send(CU8(0x70, 0x02), 2);
  spi_delay10us_send(CU8(0x72, 0x2f), 2);

}


