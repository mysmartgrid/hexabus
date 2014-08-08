#include "contiki-hexabus-lowlevel.h"

#include "contiki.h"
#include "default-uart.h"
#include "stm32l1xx.h"
#include "dev/watchdog.h"

void configure_system_clock(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;

	MODIFY_REG(PWR->CR, PWR_CR_VOS, PWR_CR_VOS_0);

	RCC->CR = RCC_CR_HSION;
	while (!(RCC->CR & RCC_CR_HSIRDY));

#if 1
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
#else
	RCC->CFGR = RCC_CFGR_SW_HSI;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);
#endif

	__ISB();
}

void init_lowlevel(void)
{
	uart_init();
	clock_init();
	watchdog_init();
	watchdog_start();
}
