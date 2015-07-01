#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include <avr/io.h>

//Key for calculating checksum
#define KEY_BYTE	0x39

//Device UUID
#define DEV_UUID	1

//Status LED
#define LED_DDR		DDRC
#define LED				PC0

// Button definitions
#define BUTTON_PORT		PORTD
#define BUTTON_PIN		PIND
#define BUTTON_BIT		PD5

uint16_t mac_dst_pan_id;// = IEEE802154_CONF_PANID;
uint16_t mac_src_pan_id;// = IEEE802154_CONF_PANID;


#endif /* BOOTLOADER_H_ */
