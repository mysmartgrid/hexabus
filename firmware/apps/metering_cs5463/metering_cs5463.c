#include "metering_cs5463.h"

#include "endpoint_registry.h"
#include "endpoints.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx.h"

#define MODIFY_PART(reg, bit, mask, val) reg = ((reg) & ~((mask) << (bit))) | ((val) << (bit));

#define SPI_PORT     GPIOB
#define SPI_PERIPH   SPI2
#define SPI_PIN_SS   12
#define SPI_PIN_MOSI 15
#define SPI_PIN_MISO 14
#define SPI_PIN_SCK  13
#define SPI_AF       GPIO_AF_SPI2

#define IRQ_PORT       GPIOC
#define IRQ_PIN_ZC     0
#define IRQ_PIN_PM     1
#define IRQ_NUM_ZC     6
#define IRQ_NUM_PM     7
#define IRQ_HANDLER_ZC EXTI0_IRQHandler
#define IRQ_HANDLER_PM EXTI1_IRQHandler

#define V_FACTOR    334.23184f // = 250mV / ((91+680)Ohm / (91+680+560k+470k)Ohm)
#define I_FACTOR    1.355288f  // = 250mV / (250W / (230V * sqrt(2) * 0.24Ohm))
#define P_FACTOR    (I_FACTOR * V_FACTOR)
#define SAMPLE_RATE 4000       // = sample rate at MCLK=4.096MHz with default K=1, N=4000



static void spi_init(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

	MODIFY_PART(SPI_PORT->MODER, 2 * SPI_PIN_SS,   GPIO_MODER_MODER0, GPIO_Mode_OUT);
	MODIFY_PART(SPI_PORT->MODER, 2 * SPI_PIN_SCK,  GPIO_MODER_MODER0, GPIO_Mode_AF);
	MODIFY_PART(SPI_PORT->MODER, 2 * SPI_PIN_MISO, GPIO_MODER_MODER0, GPIO_Mode_AF);
	MODIFY_PART(SPI_PORT->MODER, 2 * SPI_PIN_MOSI, GPIO_MODER_MODER0, GPIO_Mode_AF);

	MODIFY_PART(SPI_PORT->OTYPER, SPI_PIN_SS,   GPIO_OTYPER_OT_0, GPIO_OType_PP);
	MODIFY_PART(SPI_PORT->OTYPER, SPI_PIN_SCK,  GPIO_OTYPER_OT_0, GPIO_OType_PP);
	MODIFY_PART(SPI_PORT->OTYPER, SPI_PIN_MOSI, GPIO_OTYPER_OT_0, GPIO_OType_PP);

	MODIFY_PART(SPI_PORT->OSPEEDR, 2 * SPI_PIN_SS,   GPIO_OSPEEDER_OSPEEDR0, GPIO_Speed_40MHz);
	MODIFY_PART(SPI_PORT->OSPEEDR, 2 * SPI_PIN_SCK,  GPIO_OSPEEDER_OSPEEDR0, GPIO_Speed_40MHz);
	MODIFY_PART(SPI_PORT->OSPEEDR, 2 * SPI_PIN_MOSI, GPIO_OSPEEDER_OSPEEDR0, GPIO_Speed_40MHz);

	MODIFY_PART(SPI_PORT->AFR[SPI_PIN_SCK / 8],  4 * (SPI_PIN_SCK % 8),  GPIO_AFRL_AFRL0, SPI_AF);
	MODIFY_PART(SPI_PORT->AFR[SPI_PIN_MISO / 8], 4 * (SPI_PIN_MISO % 8), GPIO_AFRL_AFRL0, SPI_AF);
	MODIFY_PART(SPI_PORT->AFR[SPI_PIN_MOSI / 8], 4 * (SPI_PIN_MOSI % 8), GPIO_AFRL_AFRL0, SPI_AF);

#if MCK != 32000000
# error
#endif
	// at 32MHz clock, divide by 2**4 for 2MBit/s
	SPI_PERIPH->CR1 = SPI_CR1_SPE | 4 * SPI_CR1_BR_0 | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
}

static void spi_open(void)
{
	SPI_PORT->ODR &= ~(1 << SPI_PIN_SS);
}

static uint8_t spi_transfer(uint8_t val)
{
	SPI_PERIPH->DR = val;
	while (!(SPI_PERIPH->SR & SPI_SR_TXE));
	return SPI_PERIPH->DR;
}

static void spi_close(void)
{
	SPI_PORT->ODR |= 1 << SPI_PIN_SS;
}

static uint8_t current_page;

static void ensure_page(uint8_t page)
{
	if (current_page != page) {
		spi_transfer((1 << 6) | (31 << 1));
		spi_transfer(page);
		current_page = page;
	}
}

static void reg_write(uint8_t page, uint8_t reg, uint32_t value)
{
	uint8_t v2 = (value >> 16) & 0xFF;
	uint8_t v1 = (value >>  8) & 0xFF;
	uint8_t v0 = (value >>  0) & 0xFF;

	spi_open();

	ensure_page(page);

	// Register write: 0 1 A4 A3 A2 A1 A0 0
	spi_transfer((1 << 6) | (reg << 1));
	spi_transfer(v2);
	spi_transfer(v1);
	spi_transfer(v0);

	spi_close();
}

static uint32_t reg_read(uint8_t page, uint8_t reg)
{
	uint8_t b2, b1, b0;

	spi_open();

	ensure_page(page);

	// Register read: 0 0 A4 A3 A2 A1 A0 0
	spi_transfer(reg << 1);
	b2 = spi_transfer(0xff);
	b1 = spi_transfer(0xff);
	b0 = spi_transfer(0xff);

	spi_close();

	return (b2 << 16) | (b1 << 8) | b0;
}



static void interrupt_init(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	MODIFY_PART(IRQ_PORT->MODER, 2 * IRQ_PIN_PM, GPIO_MODER_MODER0, GPIO_Mode_IN);
	MODIFY_PART(IRQ_PORT->MODER, 2 * IRQ_PIN_ZC, GPIO_MODER_MODER0, GPIO_Mode_IN);

	MODIFY_PART(EXTI->IMR, IRQ_PIN_PM, EXTI_IMR_MR0, EXTI_IMR_MR0);
	MODIFY_PART(EXTI->RTSR, IRQ_PIN_PM, EXTI_IMR_MR0, EXTI_RTSR_TR0);

	MODIFY_PART(EXTI->IMR, IRQ_PIN_ZC, EXTI_IMR_MR0, EXTI_IMR_MR0);
	MODIFY_PART(EXTI->RTSR, IRQ_PIN_ZC, EXTI_IMR_MR0, EXTI_RTSR_TR0);
	MODIFY_PART(EXTI->FTSR, IRQ_PIN_ZC, EXTI_IMR_MR0, EXTI_FTSR_TR0);

	MODIFY_REG(SYSCFG->EXTICR[0], SYSCFG_EXTICR1_EXTI0, SYSCFG_EXTICR1_EXTI0_PC);
	MODIFY_REG(SYSCFG->EXTICR[0], SYSCFG_EXTICR1_EXTI1, SYSCFG_EXTICR1_EXTI1_PC);

	NVIC_EnableIRQ(IRQ_NUM_ZC);
	NVIC_SetPriority(IRQ_NUM_ZC, 32);

	NVIC_EnableIRQ(IRQ_NUM_PM);
	NVIC_SetPriority(IRQ_NUM_PM, 32);
}

static void cs5463_enable_irq(void)
{
	// enable data ready interrupt
	reg_write(0, 26, 0x800000);
}

static void cs5463_disable_irq(void)
{
	reg_write(0, 15, 0xFFFFFF);
	reg_write(0, 26, 0);
}

static void cs5463_init(void)
{
	spi_open();

	// soft-reset the chip
	spi_transfer(0x80);
	current_page = 0xff;
	ensure_page(0);

	// e3mode -> voltage sign
	// measure ac frequency
	reg_write(0, 18, 0x09);
	// set interrupt signal to positive pulse
	// clock divider 1
	reg_write(0, 0, (1 << 12) | (1 << 11) | 1);

	cs5463_enable_irq();

	// start continuous computation
	spi_transfer(0xe8);

	spi_close();
}



static float sfixed_to_float(uint32_t fixed, unsigned int_part)
{
	float magnitude = (fixed & ~(1 << 23)) / ((1 << (23 - int_part)) * 1.0f);

	if (fixed & (1 << 23))
		return -1 + magnitude;
	else
		return magnitude;
}

static float ufixed_to_float(uint32_t fixed, unsigned int_part)
{
	return fixed / ((1 << (24 - int_part)) * 1.0f);
}



static float active_power;
static float rms_current;
static float rms_voltage;
static float frequency;
static float temperature;
static float reactive_power;
static float power_factor;
static float apparent_power;
static float fundamental_active_power;
static float fundamental_reactive_power;

void IRQ_HANDLER_PM(void)
{
	EXTI->PR |= EXTI_PR_PR0 << IRQ_PIN_PM;

	cs5463_disable_irq();

	active_power = sfixed_to_float(reg_read(0, 10), 0) * P_FACTOR;
	rms_current = ufixed_to_float(reg_read(0, 11), 0) * I_FACTOR;
	rms_voltage = ufixed_to_float(reg_read(0, 12), 0) * V_FACTOR;
	frequency = sfixed_to_float(reg_read(0, 13), 0) * SAMPLE_RATE;
	temperature = sfixed_to_float(reg_read(0, 19), 7);
	reactive_power = ufixed_to_float(reg_read(0, 24), 1) * P_FACTOR;
	power_factor = sfixed_to_float(reg_read(0, 25), 0);
	apparent_power = ufixed_to_float(reg_read(0, 27), 1) * P_FACTOR;
	fundamental_active_power = sfixed_to_float(reg_read(0, 30), 0) * P_FACTOR;
	fundamental_reactive_power = sfixed_to_float(reg_read(0, 31), 0) * P_FACTOR;

	cs5463_enable_irq();
}

void IRQ_HANDLER_ZC(void)
{
	EXTI->PR |= EXTI_PR_PR0 << IRQ_PIN_ZC;

	metering_cs5463_on_zero_cross();
}



void metering_cs5463_init()
{
	spi_init();
	interrupt_init();
	cs5463_init();

	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_POWER_METER, "Active power", read_active_power, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_METERING_RMS_CURRENT, "RMS current", read_rms_current, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_METERING_RMS_VOLTAGE, "RMS voltage", read_rms_voltage, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_METERING_FREQUENCY, "Frequency", read_frequency, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_TEMPERATURE, "Temperature", read_temperature, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_METERING_REACTIVE_POWER, "Reactive power", read_reactive_power, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_METERING_POWER_FACTOR, "Power factor", read_power_factor, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_METERING_APPARENT_POWER, "Apparent power", read_apparent_power, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_METERING_FUNDAMENTAL_ACTIVE_POWER, "Fundamental active power", read_fundamental_active_power, 0);
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_METERING_FUNDAMENTAL_REACTIVE_POWER, "Fundamental reactive power", read_fundamental_reactive_power, 0);
}

__attribute__((weak))
void metering_cs5463_on_zero_cross(void)
{
}
