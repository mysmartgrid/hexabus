#include "stm32l1xx.h"
#include "contiki-hexabus-lowlevel.h"
#include "default-uart.h"
#include <stdio.h>
#include <string.h>

#include "sys/clock.h"
#include "dev/watchdog.h"
#include "rf230bb.h"

#include "nvm.h"

uint8_t encryption_enabled = 1;

void get_aes128key_from_eeprom(char* foo)
{
}

int _write(int fd, const void* data, size_t len)
{
	uart_transmit(data, len);
	return len;
}

int _read(int fd, void* data, size_t len)
{
	return 0;
}

int main(void)
{
	init_lowlevel();
	PWR->CR |= PWR_CR_DBP;

	setvbuf(stdout, 0, _IONBF, 0);
	printf("\r\nreboot %u\r\n", (unsigned) ++RTC->BKP0R);

	rf230_init();

	for (unsigned i = 0;; i++) {
		watchdog_periodic();
	}

	while (1);
}
