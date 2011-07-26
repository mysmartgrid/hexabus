
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/fuse.h>


#include "spi.h"

#include "Descriptors.h"

#include "eeprom_variables.h"

#include <LUFA/Drivers/USB/USB.h>

#include "Bootloader.h"


/** Contains the current baud rate and other settings of the first virtual serial port. This must be retained as some
 *  operating systems will not open the port unless the settings can be set successfully.
 */
CDC_Line_Coding_t LineEncoding = { .BaudRateBPS = BAUDRATE,
                                   .CharFormat  = STOPBITS,
                                   .ParityType  = PARITYMODE,
                                   .DataBits    = DATABITS
								};

/** Flag to indicate if the bootloader should be running, or should exit and allow the application code to run
 *  via a watchdog reset. When cleared the bootloader will exit, starting the watchdog and entering an infinite
 *  loop until the AVR restarts and the application runs.
 */
bool RunBootloader = true;
/* Counter for the timer interrupt */
volatile unsigned int Counter;
/* Indicates if timer has exceeded the maximum time he should stay in bootloader mode */
volatile bool timer;

/* Jump address to contiki */
void (*Start_Program)(void)=(void *)0x0000;

/*-------------------------------------------------------------------------*/
/*----------------------Configuration of the .elf file---------------------*/
#if 1
/* The proper way to set the signature is */
#include <avr/signature.h>
#else
/* Older avr-gcc's may not define the needed SIGNATURE bytes. Do it manually if you get an error */
typedef struct {const unsigned char B2;const unsigned char B1;const unsigned char B0;} __signature_t;
#define SIGNATURE __signature_t __signature __attribute__((section (".signature")))
SIGNATURE = {
  .B2 = 0x82,//SIGNATURE_2, //AT90USB1287
  .B1 = 0x97,//SIGNATURE_1, //128KB flash
  .B0 = 0x1E,//SIGNATURE_0, //Atmel
};
#endif

FUSES ={.low = 0xd0, .high = 0x90, .extended = 0xff,};

volatile uint8_t ee_mem[EESIZE] EEMEM =
{
  0x00,                                                 // ee_dummy
  0x02, 0x11, 0x22, 0xff, 0xfe, 0x33, 0x44, 0x55,       // ee_mac_addr
  0x00, 0x00,                                           // ee_pan_addr
  0xCD, 0xAB,                                           // ee_pan_id
  0x00,                                                 // ee_bootloader_flag
  0x00, 0x00,	 										//ee_bootloader_crc
  0x01,													//ee_first_run
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //ee_encryption_key
};

/** Configures all hardware required for the bootloader. */
void SetupHardware(void)
{
	/* Relocate the interrupt vector table to the bootloader section */
	MCUCR = (1 << IVCE);
	MCUCR = (1 << IVSEL);

	transceiver_init();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Initialize USB Subsystem */
	USB_Init();
	timer_init();
}

/*
 * Initializing the timer
 */
void timer_init(void) {
	TCCR0B = ( 1 << CS02 ) | ( 1 << CS00 ); // Teiler: 1024
	TIMSK0 = ( 1 << TOIE0 );         // Overflow Interrupt einschalten
}

/*
 * Initializing the transceiver with a clock-rate of 8 MHz,
 * because we need this clock for USB.
 */
void transceiver_init(void) {
	/* init spi */
	spi_masterInit();

	//IO Pins
	DDR_SPI |= (1 << SLP_TR); //Enable SLP_TR as output.
	DDR_SPI |= (1 << AT_RST); //Enable RST as output.

	//Get the SLP_TR and RST right.to leave P_ON State
	_delay_us(330);
	RST_LOW;
	SLP_TR_LOW;
	_delay_us(1);
	RST_HIGH;
	_delay_us(26);
	spi_regWrite(0x03, 0x14); // Setting clock to 8 MHz
	spi_regWrite(0x02, 0x08); // TRX_OFF
	_delay_us(330);

}

/**
 * Event handler for the USB_ConfigurationChanged event. This configures the device's endpoints ready
 * to relay data to and from the attached USB host.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	/* Setup CDC Notification, Rx and Tx Endpoints */
	Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPNUM, EP_TYPE_INTERRUPT,
	                           ENDPOINT_DIR_IN, CDC_NOTIFICATION_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	Endpoint_ConfigureEndpoint(CDC_TX_EPNUM, EP_TYPE_BULK,
	                           ENDPOINT_DIR_IN, CDC_TXRX_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	Endpoint_ConfigureEndpoint(CDC_RX_EPNUM, EP_TYPE_BULK,
	                           ENDPOINT_DIR_OUT, CDC_TXRX_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
	/* Process CDC specific control requests */
	switch (USB_ControlRequest.bRequest)
	{
		case REQ_GetLineEncoding:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();

				/* Write the line coding data to the control endpoint */
				Endpoint_Write_Control_Stream_LE(&LineEncoding, sizeof(CDC_Line_Coding_t));
				Endpoint_ClearOUT();
			}

			break;
		case REQ_SetLineEncoding:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();

				/* Read the line coding data in from the host into the global struct */
				Endpoint_Read_Control_Stream_LE(&LineEncoding, sizeof(CDC_Line_Coding_t));
				Endpoint_ClearIN();
			}

			break;
	}
}

/**
 * Retrieves the next byte from the host in the CDC data OUT endpoint, and clears the endpoint bank if needed
 * to allow reception of the next data packet from the host.
 *
 * \return Next received byte from the host in the CDC data OUT endpoint
 */
static uint8_t FetchNextCommandByte(void)
{
	/* Select the OUT endpoint so that the next data byte can be read */
	Endpoint_SelectEndpoint(CDC_RX_EPNUM);

	/* If OUT endpoint empty, clear it and wait for the next packet from the host */
	while (!(Endpoint_IsReadWriteAllowed()))
	{
		Endpoint_ClearOUT();

		while (!(Endpoint_IsOUTReceived()))
		{
			if (USB_DeviceState == DEVICE_STATE_Unattached)
			  return 0;
		}
	}

	/* Fetch the next byte from the OUT endpoint */
	return Endpoint_Read_Byte();
}

/**
 * Writes the next response byte to the CDC data IN endpoint, and sends the endpoint back if needed to free up the
 * bank when full ready for the next byte in the packet to the host.
 *
 * \param[in] Response  Next response byte to send to the host
 */
static void WriteNextResponseByte(const uint8_t Response)
{
	/* Select the IN endpoint so that the next data byte can be written */
	Endpoint_SelectEndpoint(CDC_TX_EPNUM);

	/* If IN endpoint full, clear it and wait until ready for the next packet to the host */
	if (!(Endpoint_IsReadWriteAllowed()))
	{
		Endpoint_ClearIN();

		while (!(Endpoint_IsINReady()))
		{
			if (USB_DeviceState == DEVICE_STATE_Unattached)
			  return;
		}
	}

	/* Write the next byte to the OUT endpoint */
	Endpoint_Write_Byte(Response);

	/* Select the IN endpoint */
	Endpoint_SelectEndpoint(CDC_TX_EPNUM);

	/* Remember if the endpoint is completely full before clearing it */
	bool IsEndpointFull = !(Endpoint_IsReadWriteAllowed());

	/* Send the endpoint data to the host */
	Endpoint_ClearIN();

	/* If a full endpoint's worth of data was sent, we need to send an empty packet afterwards to signal end of transfer */
	if (IsEndpointFull)
	{
		while (!(Endpoint_IsINReady()))
		{
			if (USB_DeviceState == DEVICE_STATE_Unattached)
			  return;
		}

		Endpoint_ClearIN();
	}

	/* Wait until the data has been sent to the host */
	while (!(Endpoint_IsINReady()))
	{
		if (USB_DeviceState == DEVICE_STATE_Unattached) {
			/* Select the OUT endpoint */
			Endpoint_SelectEndpoint(CDC_RX_EPNUM);

			/* Acknowledge the command from the host */
			Endpoint_ClearOUT();

			return;
		}
	}

	/* Select the OUT endpoint */
	Endpoint_SelectEndpoint(CDC_RX_EPNUM);

	/* Acknowledge the command from the host */
	Endpoint_ClearOUT();
}

/**
 * Programming the microcontroller over the virtual serial line.
 * 				Command										Return
 * Commands:	e			Exit							\r
 * 				p			Program flash from HEX-File		\r
 */
void SerialControl(void) {
	/* Select the OUT endpoint */
	Endpoint_SelectEndpoint(CDC_RX_EPNUM);

	/* Check if endpoint has a command in it sent from the host */
	if (!(Endpoint_IsOUTReceived()))
	  return;

	/* Read in the bootloader command (first byte sent from host) */
	uint8_t Command = FetchNextCommandByte();

	if (Command == 'e')	{
		RunBootloader = false;

		/* Send confirmation byte back to the host */
		WriteNextResponseByte(0x05);
	}
	else if (Command == 'p') {

		/* Send confirmation byte back to the host */
		WriteNextResponseByte(0x01);
		// Everything was correct
		if(ProgramFlash() == 1) {
			WriteNextResponseByte(0x05);
			RunBootloader = false;
		}
		// A wrong line was sent
		else {
			WriteNextResponseByte(0x00);
		}

	}
	else if (Command != 27)	{
		/* Unknown (non-sync) command, return fail code */
		WriteNextResponseByte('?');
	}
}

/*
 * Flashing the LED
 */
void flash_led(void)
{
	LED_DDR ^= _BV(LED);

}

/**
 * Programs the flash from an Intel Hex-File. The Hex-File must be transmitted line by line.
 * After each line the microcontroller sends an answer to show if the checksum was
 * correct or not. If the checksum was wrong the line must be resent.
 * If the checksum is correct it returns 0x05 to the host otherwise it returns 0x06 to the host.
 * After the host gets this line he can send the next line or resent the last line.
 * A (german) description of the Intel Hex format can be found here:
 * http://www.schulz-koengen.de/biblio/intelhex.htm
 *
 * @return -1	if a line starts with any other character than ':'
 * 			1	if a line with the "End of File Record" was sent
 */
int ProgramFlash() {

	uint8_t buffer[255];	// Buffer for reading a line
	uint8_t bufferPos;		// Position in buffer

	uint8_t temp;			// Temp variable
	bool nextLine;			// Indicates if there is a next line or not
	uint16_t pagePos;		// Position in the boot-page buffer
	uint8_t dataLength;		// Number of data-bytes of the current line
	uint16_t startAddress;	// Start address for the data of the current data-line
	uint16_t currentAddress;// Current address for the data of the current data-line
	uint8_t type;			// Record type of the current line.
	uint8_t checksum;		// In this variable the checksum of one line is calculated

	startAddress = 0xFFFF;	// Invalid value, because no register is selected

	nextLine = 1;
	pagePos = 0;

	while(nextLine) {
		// Checking if the begin of the line is correct, otherwise we break up.
		if(FetchNextCommandByte() != ':')
			return -1;

		dataLength = FetchNextCommandByte();					// Line length
		checksum = dataLength;

		temp = FetchNextCommandByte();
		checksum += temp;
		currentAddress = ( temp << 8 );							// High byte
		temp = FetchNextCommandByte();
		checksum += temp;
		currentAddress = currentAddress | temp;					// Adding low byte

		type = FetchNextCommandByte();							// Record type
		checksum += type;

		// Reading a complete hex-line
		int i;
		for(i = 0; i < dataLength; i++) {
			buffer[i] = FetchNextCommandByte();
			checksum += buffer[i];
		}

		// Calculating checksum
		checksum = 0xFF - checksum + 1;

		if(checksum == FetchNextCommandByte()) {
			// If the record type indicates that this is the last line.
			if(type == 0x01)
				nextLine = 0;

			// Only do something if the line contains data bytes
			if(dataLength) {
				// Setting new startaddress
				if(startAddress == 0xFFFF) {
					// Calculating first position in page
					temp = currentAddress / PAGESIZE;
					startAddress = PAGESIZE * temp;;
					// If we want to write a new page we must first erase the data
					if(pagePos == 0)
						boot_erase_page(startAddress);						// Erase page
				}
				// Startaddress is outside of the current page
				else if(startAddress > currentAddress || ( startAddress + PAGESIZE ) <= currentAddress) {
					boot_write_page(startAddress);	// Write the page from the buffer to the flash
					pagePos = 0;
					// Calculating first position in page
					temp = currentAddress / PAGESIZE;
					startAddress = PAGESIZE * temp;
					// If we want to write a new page we must first erase the data
					if(pagePos == 0)
						boot_erase_page(startAddress);						// Erase page
				}
				pagePos = currentAddress - startAddress;

				for(bufferPos = 0; bufferPos < dataLength; bufferPos+=2, pagePos+=2) {

					// If the current page is full we need to write that page to the flash and start a new one
					if(pagePos >= PAGESIZE) {

						boot_write_page(startAddress);	// Write the page from the buffer to the flash
						startAddress = currentAddress + bufferPos;
						pagePos = 0;
						boot_erase_page(startAddress);						// Erase page
					}
					// Setting up two bites to a little-endian word since we have to write words to the buffer
					temp = buffer[bufferPos];
					checksum += temp;
					uint16_t w = temp;

					temp = buffer[bufferPos+1];
					checksum += temp;
					w += (temp) << 8;

					boot_write_buffer(currentAddress, bufferPos, w);	// Write page buffer
				}
				WriteNextResponseByte(0x05);	// Write back, that everything went fine
			}
		}
		// If checksum didn't match then return 0x06
		else {
			WriteNextResponseByte(0x06);
			bufferPos = 0;
			pagePos -= dataLength;
		}
	}
	boot_write_page(startAddress);	// Write the page from the buffer to the flash
	AVR_ENTER_CRITICAL_REGION();
	eeprom_write_word((uint16_t *)EE_BOOTLOADER_CRC, calc_crc());	// Write CRC to eeprom
	eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0xFF); // Write Bootloader-Flag to eeprom
	AVR_LEAVE_CRITICAL_REGION();
	return 1;
}

/**
 * Erasing a page of the flash
 *
 * @param (Byte-) Address of the page
 */
void boot_erase_page(uint32_t address) {
	boot_page_erase(address);
  	boot_spm_busy_wait();
	boot_rww_enable();
	return;
}

/**
 * Writing words to the buffer
 *
 * @param	baseaddr	(Byte-) Address of the page
 * @param	pageaddr	Position (in bytes) in the page
 * @param	word		Word to write
 */
void boot_write_buffer(uint32_t baseaddr, uint16_t pageaddr, uint16_t word) {
	boot_page_fill(baseaddr + pageaddr, word);
	return;
}

/**
 * Writing data from the buffer to the page
 *
 * @param	address		(Byte-) Address of the page
 */
void boot_write_page(uint32_t address) {
	boot_page_write(address);	// Store buffer in flash page.
    boot_spm_busy_wait();		// Wait until the memory is written.
	boot_rww_enable();			// Reenable RWW-section again. We need this if we want to jump back
								// to the application after bootloading.
	return;
}

/*
 * Calculates a crc of the flash-memory
 *
 * @return the crc
 */
uint16_t calc_crc(void) {
	uint16_t crc;

	uint16_t c, i;
	prog_uchar *ptr;
	uint8_t byte = 0;
	uint8_t crcbit = 0;
	uint8_t databit = 0;
	ptr = 0;
	// init to all ones
	crc = 0xFFFF;

	for (c = 0; c < BOOTLOADER_START; c++)
	{
		byte = pgm_read_byte(ptr++);

		for (i = 0; i < 8; i++)
		{
		  crcbit = (crc & 0x8000) ? 1 : 0;
		  databit = (byte & 0x80) ? 1 : 0;
		  crc = crc << 1;
		  byte = byte << 1;

	          if (crcbit != databit)
	            crc = crc ^ 0x1021;
		}
	}

	return crc;
}


/*
 * Timer interrupt
 * Sets "timer" to true after Counter == 1000 and stops the
 * timer interrupt.
 */
ISR(TIMER0_OVF_vect) {
	  Counter++;

	  if(Counter%5 == 0)
		  flash_led();
	  if( Counter == 1000 ) {
	    Counter = 0;
	    timer = true;
	    LED_DDR |= _BV(LED);
	  }
}

int main(void) {

	ee_mem[0];
    /* Disable watchdog if enabled by bootloader/fuses */
	MCUSR = 0;
	wdt_disable();
	cli();
	// Check if button is pressed, if it is pressed we always go into the bootloader
	if(bit_is_clear(BUTTON_PIN, BUTTON_BIT)) {
		AVR_ENTER_CRITICAL_REGION( );
		eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0x00);
		AVR_LEAVE_CRITICAL_REGION();
	}

	if(eeprom_read_byte((uint8_t *)EE_BOOTLOADER_FLAG)) {
		// Read crc from eeprom
		uint16_t crc;
		AVR_ENTER_CRITICAL_REGION();
		crc = eeprom_read_word ((uint16_t *)EE_BOOTLOADER_CRC);
		AVR_LEAVE_CRITICAL_REGION();
		// Compare crc
		if(crc == calc_crc()) {
			Start_Program();
		}
	}

	Counter = 0;
	timer = false;

	/* Setup hardware required for the bootloader */
	SetupHardware();

	/* Enable global interrupts so that the USB stack can function */
	sei();

	// If a packet is received then start the flash program
	if(!timer) {

		while (RunBootloader)
		{
			// If timer is true we leave the bootloader-loop
			if(timer) {
				RunBootloader = false;
				AVR_ENTER_CRITICAL_REGION();
				eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0xFF); // Write Bootloader-Flag to eeprom
				AVR_LEAVE_CRITICAL_REGION();
			}
			SerialControl();
			USB_USBTask();
		}
	}

	/* Disconnect from the host - USB interface will be reset later along with the AVR */
	USB_Detach();

	AVR_ENTER_CRITICAL_REGION();
	eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0xFF); // Write Bootloader-Flag to eeprom
	AVR_LEAVE_CRITICAL_REGION();

	/* Enable the watchdog and force a timeout to reset the AVR */
	wdt_enable(WDTO_250MS);

	for (;;);

}
