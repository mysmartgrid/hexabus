#include "platform_init.h"

#include <stdio.h>

#include "contiki.h"
#include "default-uart.h"
#include "stm32l1xx.h"
#include "dev/leds.h"
#include "dev/watchdog.h"

void configure_system_clock(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;

	MODIFY_REG(PWR->CR, PWR_CR_VOS, PWR_CR_VOS_0);

	RCC->CR = RCC_CR_HSION;
	while (!(RCC->CR & RCC_CR_HSIRDY));

	RCC->CFGR =
		RCC_CFGR_PLLMUL4 |
		RCC_CFGR_PLLDIV2 |
		RCC_CFGR_PLLSRC_HSI;

	RCC->CR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));

	FLASH->ACR = FLASH_ACR_ACC64;
	FLASH->ACR |= FLASH_ACR_LATENCY | FLASH_ACR_PRFTEN;

	RCC->CFGR = RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

void platform_init()
{
	leds_init();
	uart_init();

	setvbuf(stdin, 0, _IONBF, 0);
	setvbuf(stdout, 0, _IONBF, 0);

	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
}
