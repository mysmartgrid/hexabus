#include "contiki-conf.h"
#include "dev/leds.h"

#include "stm32l1xx.h"

void leds_arch_init(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

	GPIOB->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
	GPIOB->MODER |= (1 * GPIO_MODER_MODER6_0) | (1 * GPIO_MODER_MODER7_0);

	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7);

	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7);
}

unsigned char leds_arch_get(void)
{
	return (GPIOB->ODR & (1 << 6) ? LEDS_RED : 0)
		| (GPIOB->ODR & (1 << 7) ? LEDS_GREEN : 0);
}

void leds_arch_set(unsigned char leds)
{
	GPIOB->ODR &= ~((1 << 6) | (1 << 7));
	GPIOB->ODR |= (leds & LEDS_RED ? 1 << 6 : 0) | (leds & LEDS_GREEN ? 1 << 7 : 0);
}
