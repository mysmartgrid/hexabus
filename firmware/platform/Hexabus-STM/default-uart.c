#include "default-uart.h"
#include "stm32l1xx.h"
#include "stm32l1xx_gpio.h"

void uart_init(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

	GPIOA->MODER &= ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10);
	GPIOA->MODER |= (GPIO_Mode_AF << 18) | (GPIO_Mode_AF << 20);

	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10);

	GPIOA->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR9 | GPIO_OSPEEDER_OSPEEDR10);
	GPIOA->OSPEEDR |= (GPIO_Speed_40MHz << 18) | (GPIO_Speed_40MHz << 20);

	GPIOA->AFR[1] &= ~(GPIO_AFRH_AFRH9 | GPIO_AFRH_AFRH10);
	GPIOA->AFR[1] |= (GPIO_AF_USART1 << 4) | (GPIO_AF_USART1 << 8);

	// set for 115200 at 32MHz peripheral clock, BRR = 8.6875
	USART1->BRR = (17 << 4) | (6 << 0);
	USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
}

void uart_transmit(const void* data, unsigned length)
{
	const uint8_t* ubuf = data;
	const uint8_t* end = ubuf + length;

	while (ubuf != end) {
		while (!(USART1->SR & USART_SR_TXE));
		USART1->DR = *ubuf++;
	}
}
