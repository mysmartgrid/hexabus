#ifndef EEPROM_VAR_H_
#define EEPROM_VAR_H_

#include <avr/eeprom.h>

#define EESIZE   4096      // Maximum size of EEPROM

#define EE_DUMMY   		0x000  	                                                // Dummy element (Address 0 should not be used)

#define EE_MAC_ADDR 		0x001			  									// EUI-64 MAC-Address of the device  (8 Byte)
#define EE_MAC_ADDR_SIZE 		8

#define EE_PAN_ADDR  		( EE_MAC_ADDR + EE_MAC_ADDR_SIZE )					// 802.15.4 short address (2 Byte)
#define EE_PAN_ADDR_SIZE		2

#define EE_PAN_ID  		( EE_PAN_ADDR + EE_PAN_ADDR_SIZE )						// 802.15.4 PAN-ID (2 Byte)
#define EE_PAN_ID_SIZE			2

#define EE_BOOTLOADER_FLAG      ( EE_PAN_ID + EE_PAN_ID_SIZE )                  // Bootloader Flag for reset
#define EE_BOOTLOADER_FLAG_SIZE         1

#define EE_BOOTLOADER_CRC		( EE_BOOTLOADER_FLAG + EE_BOOTLOADER_FLAG_SIZE )// CRC over image, is checked at startup
#define EE_BOOTLOADER_CRC_SIZE 	2

#define EE_FIRST_RUN_FLAG		( EE_BOOTLOADER_CRC + EE_BOOTLOADER_CRC_SIZE )	//flag which indicates the first startup, it is used for generating a random pan_id once at first startup
#define EE_FIRST_RUN_FLAG_SIZE 	1

#define EE_ENCRYPTION_KEY		( EE_FIRST_RUN_FLAG + EE_FIRST_RUN_FLAG_SIZE )	//128bit AES key for encryption
#define EE_ENCRYPTION_KEY_SIZE 16


#endif /* EEPROM_VAR_H_ */

