#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_



#define PAGESIZE	256	/**< Size (in Bytes) of a flash page */
#define BOOTLOADER_START	0xF000	/**< Start of the bootloader section */
/**
 * Settings for the virtual serial port. Change this if you need other settings for the serial connection
 */
#define	BAUDRATE	0U	/**< Baud rate of the serial port, in bits per second. Since
							this is a virtual serial port, this value can stay 0 */

#define STOPBITS	1U	/**< Number of stop bits after each frame:
							0 = One stop bit,
							1 = 1.5 stop bits,
							2 = 2 stop bits */

#define PARITYMODE	0U	/**< Character format of the serial port.
							0 = No parity bit mode on each frame,
							1 = Odd parity bit mode on each frame,
							2 = Even parity bit mode on each frame,
							3 = Mark parity bit mode on each frame,
							4 = Space parity bit mode on each frame */

#define DATABITS	8U	/**< Number of data bits */


/** CDC Class specific request to get the current virtual serial port configuration settings. */
#define REQ_GetLineEncoding          0x21

/** CDC Class specific request to set the current virtual serial port configuration settings. */
#define REQ_SetLineEncoding          0x20

/** Type define for the virtual serial port line encoding settings, for storing the current USART configuration
 *  as set by the host via a class specific request.
 */
typedef struct {
	uint32_t BaudRateBPS; /**< Baud rate of the virtual serial port, in bits per second */
	uint8_t  CharFormat; /**< Character format of the virtual serial port, a value from the
	                      *   CDCDevice_CDC_LineCodingFormats_t enum
	                      */
	uint8_t  ParityType; /**< Parity setting of the virtual serial port, a value from the
	                      *   CDCDevice_LineCodingParity_t enum
	                      */
	uint8_t  DataBits; /**< Bits of data per character of the virtual serial port */
} CDC_Line_Coding_t;

// Button definitions
#define BUTTON_PORT		PORTE
#define BUTTON_PIN		PINE
#define BUTTON_BIT		PE2
#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))

#define LED_DDR  DDRA
#define LED_PORT PORTA
#define LED_PIN  PINA
#define LED      PA3

/** This macro will protect the following code from interrupts.*/
#define AVR_ENTER_CRITICAL_REGION( ) {uint8_t volatile saved_sreg = SREG; cli( )

/** This macro must always be used in conjunction with AVR_ENTER_CRITICAL_REGION
    so that interrupts are enabled again.*/
#define AVR_LEAVE_CRITICAL_REGION( ) SREG = saved_sreg;}

void SetupHardware(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void SerialControl(void);
int ProgramFlash(void);
void timer_init(void);
void transceiver_init(void);

void boot_erase_page(uint32_t address);
void boot_write_buffer(uint32_t baseaddr, uint16_t pageaddr, uint16_t word);
void boot_write_page(uint32_t address);

void flash_led(void);
uint16_t calc_crc(void);
#endif /* BOOTLOADER_H_ */
