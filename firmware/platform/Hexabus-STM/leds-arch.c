#include "contiki-conf.h"
#include "dev/leds.h"

#include "stm32l1xx.h"
#include "stm32l1xx_gpio.h"

#define LED_RED_PORT GPIOC
#define LED_RED_BIT  (1 << 13)

#define LED_GREEN_PORT GPIOB
#define LED_GREEN_BIT  (1 << 2)

void leds_arch_init(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;

	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER2, GPIO_Mode_OUT * GPIO_MODER_MODER2_0);
	MODIFY_REG(GPIOB->OTYPER, GPIO_OTYPER_OT_2, GPIO_OType_PP * GPIO_OTYPER_OT_2);
	MODIFY_REG(GPIOB->PUPDR, GPIO_PUPDR_PUPDR2, GPIO_PuPd_NOPULL * GPIO_PUPDR_PUPDR2_0);

	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER13, GPIO_Mode_OUT * GPIO_MODER_MODER13_0);
	MODIFY_REG(GPIOC->OTYPER, GPIO_OTYPER_OT_13, GPIO_OType_PP * GPIO_OTYPER_OT_13);
	MODIFY_REG(GPIOC->PUPDR, GPIO_PUPDR_PUPDR13, GPIO_PuPd_NOPULL * GPIO_PUPDR_PUPDR13_0);

	leds_arch_set(0);
}

unsigned char leds_arch_get(void)
{
	return (LED_RED_PORT->ODR & LED_RED_BIT ? 0 : LEDS_RED)
		| (LED_GREEN_PORT->ODR & LED_GREEN_BIT ? 0 : LEDS_GREEN);
}

void leds_arch_set(unsigned char leds)
{
	MODIFY_REG(LED_RED_PORT->ODR, LED_RED_BIT, leds & LEDS_RED ? 0 : LED_RED_BIT);
	MODIFY_REG(LED_GREEN_PORT->ODR, LED_GREEN_BIT, leds & LEDS_GREEN ? 0 : LED_GREEN_BIT);
}
