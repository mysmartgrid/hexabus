#include "stm32l1xx.h"
#include "contiki-hexabus-lowlevel.h"
#include "default-uart.h"
#include <stdio.h>
#include <string.h>

#include "sys/clock.h"
#include "dev/watchdog.h"

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

	for (unsigned i = 0;; i++) {
		for (int i = 0; i < 1000000; i++) asm("nop");
		printf("\r%u %i", (unsigned) clock_seconds(), i);
		watchdog_periodic();
	}

	while (1);
}
