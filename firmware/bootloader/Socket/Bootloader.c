#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#include "nvm.h"
#include "sys/process.h"
#include "dev/radio.h"
#include "net/mac/framer-802154.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "rf230bb.h"

#include "Bootloader.h"


#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))

extern uint8_t _text;

extern uint8_t encryption_enabled; // = 0;
extern uint16_t mac_dst_pan_id;
extern uint16_t mac_src_pan_id;

volatile static uint16_t counter;
volatile static uint8_t timer;

/* Jump address to contiki */
void (*contiki)(void)=(void *)0x0000;

enum program_state_t {
	PSTATE_REQUESTING = 0,
	PSTATE_WAITING = 1,
	PSTATE_RECEIVED_DATA = 2,
	PSTATE_RECEIVED_FINISH = 3,
};

enum {
	HXB_B_FLASH_REQUEST = 0,
	HXB_B_FLASH_DATA = 1,
	HXB_B_FLASH_FINISH = 2,
	HXB_B_FLASH_ACK = 3,
	HXB_B_FLASH_NACK = 4,

	/* for coexistence with 6lowpan, keep all message tags below 64 */
	HXB_B_TAG = 1,
};

/* Packet definitions */
struct flash_request {
	uint8_t tag;
	uint8_t type;
	uint16_t dev_uuid;
} __attribute__((packed));

struct flash_data {
	uint8_t tag;
	uint8_t type;
	uint32_t address;
	uint8_t payload[64];
} __attribute__((packed));

struct flash_finish {
	uint8_t tag;
	uint8_t type;
	uint8_t checksum[16];
} __attribute__((packed));

struct flash_ack {
	uint8_t tag;
	uint8_t type;
	uint32_t address;
} __attribute__((packed));

struct flash_nack {
	uint8_t tag;
	uint8_t type;
	uint32_t address;
} __attribute__((packed));

void fill16(uint8_t *array, uint8_t byte) {
	for(uint8_t i=0; i<16; i++)
		array[i] = byte;
}

int verfiy_flash(void) {
	const uint8_t *ptr = 0;
	uint8_t checksum[16], expected_checksum[16];

	fill16(checksum, 0x00);

	while(ptr < &_text) {
		for(uint8_t j=0; j<16; j++) {
			checksum[j] ^= pgm_read_byte(ptr);
			ptr++;
		}

		rf230_cipher(checksum);
	}

	nvm_read_block(pgm_checksum, expected_checksum, 16);

	return memcmp(checksum, expected_checksum, 16);
}

void send_packet(void *packet, size_t len) {
	if(len != 0) {
		packetbuf_copyfrom(packet, len);
		packetbuf_set_datalen(len);
		NETSTACK_RDC.send(NULL, NULL);
		packetbuf_clear();

		for (uint8_t i = 0; i < 100 && !packetbuf_datalen(); i++) {
			_delay_ms(10);
			while (process_run()) {}
		}
	}
}

void send_request(void) {
	struct flash_request req = {
		HXB_B_TAG,
		HXB_B_FLASH_REQUEST,
		DEV_UUID,
	};

	send_packet(&req, sizeof(req));
}

void send_ack(uint32_t address) {
	struct flash_ack ack = {
		HXB_B_TAG,
		HXB_B_FLASH_ACK,
		address,
	};

	send_packet(&ack, sizeof(ack));
}

void send_nack(uint32_t address) {
	struct flash_ack nack = {
		HXB_B_TAG,
		HXB_B_FLASH_NACK,
		address,
	};

	send_packet(&nack, sizeof(nack));
}

void init_radio(void) {
	uint8_t key[16];

	NETSTACK_RADIO.init();
	NETSTACK_RDC.init();
	NETSTACK_MAC.init();

	rimeaddr_t addr = {};
	nvm_read_block(mac_addr, addr.u8, nvm_size(mac_addr));

	rf230_set_pan_addr(IEEE802154_CONF_PANID, nvm_read_u16(pan_addr), addr.u8);
	rf230_set_channel(RF_CHANNEL);

	mac_dst_pan_id = IEEE802154_CONF_PANID;
	mac_src_pan_id = IEEE802154_CONF_PANID;

	fill16(key, KEY_BYTE);

	rf230_key_setup(key);
}

uint8_t process_finish(uint32_t address, uint8_t *expected_checksum, uint8_t *checksum) {
	uint8_t sreg;
	uint8_t i,j;

	sreg = SREG;
	cli();

	// Fill remaining space with 0xFF for checksum calculation
	while(address < _text) {
		eeprom_busy_wait();
		boot_page_erase(address);
		boot_spm_busy_wait();

		for(i = 0; i < SPM_PAGESIZE; i+=16) {
			for(j = 0; j < 16; j++)
				checksum[j] ^= 0xFF;

			rf230_cipher(checksum);
		}

		address += SPM_PAGESIZE;
	}

	SREG = sreg;

	return memcmp(checksum, expected_checksum, 16);
}

void write_data(uint32_t address, uint8_t *data, uint8_t *checksum) {
	uint8_t sreg;
	uint8_t i,j;

	sreg = SREG;
	cli();

	/* First chunk of page */
	if(address % SPM_PAGESIZE == 0) {
		eeprom_busy_wait();
		boot_page_erase(address);
		boot_spm_busy_wait();
	}

	for (i = 0; i<64; i+=2) {
		uint16_t w = *data++;
		w += (*data++) << 8;
		boot_page_fill(address + i, w);
	}

	/* Last chunk of page */
	if((address + 64) % SPM_PAGESIZE == 0) {
		boot_page_write((address + 64) - SPM_PAGESIZE);
		boot_spm_busy_wait();
		boot_rww_enable();
	}

	SREG = sreg;

	for(i = 0; i < 64; i+=16) {
		for(j = 0; j < 16; j++)
			checksum[j] ^= data[i+j];

		rf230_cipher(checksum);
	}
}

void bootloader(void) {
	uint8_t finished = 0;

	uint32_t flash_address = 0;
	uint8_t checksum[16];

	fill16(checksum, 0x00);

	enum program_state_t state = PSTATE_REQUESTING;
	struct flash_data *buf_data = NULL;
	struct flash_finish *buf_finish = NULL;

	init_radio();

	timer = counter = 0;

	sei();

	do {
		switch(state) {
			// Flash request beacons
			case PSTATE_REQUESTING:
				_delay_ms(100);
				send_request();
				break;
			// Wait for new data after flashing has begun
			case PSTATE_WAITING:
				break;
			// ACK correct address writes and write to flash, NACK otherwise
			case PSTATE_RECEIVED_DATA:
				if(buf_data->address == flash_address && buf_data->address < _text+64) {
					write_data(buf_data->address, buf_data->payload, checksum);
					send_ack(buf_data->address);
					flash_address += 64;
				} else {
					send_nack(buf_data->address);
				}
				state = PSTATE_WAITING;
				break;
			// Check final checksum
			case PSTATE_RECEIVED_FINISH:
				if(process_finish(flash_address, buf_finish->checksum, checksum))
					send_nack(-1);
				else
					send_ack(-1);

				finished = 1;
				break;
		}

		// Query and categorize new packets
		if (packetbuf_datalen() > 0) {
			NETSTACK_RDC.input();

			if(packetbuf_datalen() == sizeof(struct flash_data)) {
				buf_data = packetbuf_dataptr();

				if(buf_data->tag == HXB_B_TAG && buf_data->type == HXB_B_FLASH_DATA)
					state = PSTATE_RECEIVED_DATA;
			} else if(packetbuf_datalen() == sizeof(struct flash_finish)) {
				buf_finish = packetbuf_dataptr();

				if(buf_finish->tag == HXB_B_TAG && buf_finish->type == HXB_B_FLASH_FINISH)
					state = PSTATE_RECEIVED_FINISH;
			}
		}
	} while(!finished || timer);

	cli();

	// Unset bootloader flag
	nvm_write_u8(bootloader_flag, 0xFF);
}

int main(void) {
	wdt_disable();
	cli();

	// Relocate interrupts
	uint8_t temp = MCUCR;
	MCUCR = temp | (1<<IVCE);
	MCUCR = temp | (1<<IVSEL);

	//Setup timer
	TCCR0B =( 0x00 | ( 1 << CS02 ) | ( 1 << CS00 ));
	TIMSK0 |= ( 1 << TOIE0 );

	// Set bootloader flag if button is pressed during boot
	if(bit_is_clear(BUTTON_PIN, BUTTON_BIT)) {
		nvm_write_u8(bootloader_flag, 0x00);
	}

	// Run bootloader until bootloader flag is unset and
	// a verfied image is in flash
	while (!nvm_read_u8(bootloader_flag) || !verfiy_flash()) {
		bootloader();
	}

	// Restore interrupts
	temp = MCUCR;
	MCUCR = temp | (1<<IVCE);
	MCUCR = temp & ~(1<<IVSEL);

	//Boot contiki
	contiki();
}

// TImer for LED and timeout
ISR(TIMER0_OVF_vect) {
	counter++;
	if(counter % 5 == 0)
		LED_DDR ^= _BV(LED);
	if(counter == 1000) {
		counter = 0;
		timer = 1;
		LED_DDR |= _BV(LED);
	}
}