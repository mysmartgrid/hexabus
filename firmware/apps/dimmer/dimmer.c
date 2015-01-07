#include "dimmer.h"

#include "endpoint_registry.h"
#include "endpoints.h"
#include "metering_cs5463.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx.h"

#define MODIFY_PART(reg, bit, mask, val) reg = ((reg) & ~((mask) << (bit))) | ((val) << (bit));

#define DIMMER_PIN   9
#define DIMMER_PORT  GPIOB

static void init_pins(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

	MODIFY_PART(DIMMER_PORT->MODER, 2 * DIMMER_PIN, GPIO_MODER_MODER0, GPIO_Mode_AF);
	MODIFY_PART(DIMMER_PORT->OTYPER, DIMMER_PIN, GPIO_OTYPER_OT_0, GPIO_OType_PP);
	MODIFY_PART(DIMMER_PORT->OSPEEDR, 2 * DIMMER_PIN, GPIO_OSPEEDER_OSPEEDR0, GPIO_Speed_40MHz);
	MODIFY_PART(DIMMER_PORT->PUPDR, 2 * DIMMER_PIN, GPIO_PUPDR_PUPDR0, GPIO_PuPd_NOPULL);
	MODIFY_PART(DIMMER_PORT->AFR[DIMMER_PIN / 8], 4 * (DIMMER_PIN % 8), GPIO_AFRL_AFRL0, GPIO_AF_TIM4);
}

enum {
	TYPE_CUT_ON,
	TYPE_CUT_OFF,
	TYPE_PWM
};

static uint8_t brightness_pct;
static uint8_t dimmer_type;

static void update_timer(void)
{
	switch (dimmer_type) {
	case TYPE_CUT_ON:
		TIM4->CCR4 = (100 - brightness_pct) * 10;
		break;

	case TYPE_CUT_OFF:
		TIM4->CCR4 = brightness_pct * 10;
		break;

	case TYPE_PWM:
		TIM4->CCR4 = brightness_pct * 25 / 10;
		break;
	}
}

static void init_timer(void)
{
	RCC->APB1RSTR |= RCC_APB1RSTR_TIM4RST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM4RST;
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

#if MCK != 32000000
# error
#endif
	TIM4->PSC = 319; // 1000 count per half-wave at 32MHz
	TIM4->CCER = TIM_CCER_CC4E; // channel 4 to output
	TIM4->CCMR2 = TIM_CCMR2_OC4PE; // preload internal counter

	switch (dimmer_type) {
	case TYPE_CUT_ON:
		TIM4->CCMR2 |= TIM_CCMR2_OC4M_0 * 7; // pwm mode 2
		TIM4->ARR = 1000; // span entire half-wave
		TIM4->CR1 |= TIM_CR1_OPM; // one-pulse mode
		break;

	case TYPE_CUT_OFF:
		TIM4->CCMR2 |= TIM_CCMR2_OC4M_0 * 6; // pwm mode 1
		TIM4->ARR = 1000; // span entire half-wave
		TIM4->CR1 |= TIM_CR1_OPM; // one-pulse mode
		break;

	case TYPE_PWM:
		TIM4->ARR= 250; // four timer periods per half-wave
		TIM4->CCMR2 |= TIM_CCMR2_OC4M_0 * 7; // pwm mode 2
		break;
	}

	TIM4->EGR = TIM_EGR_UG; // generate update event to initialize registers
	TIM4->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN; // preload ARR, enable
	update_timer();
}

#ifdef CONTIKI_TARGET_HEXABUS_STM
void metering_cs5463_on_zero_cross(void)
{
	if (dimmer_type != TYPE_PWM) {
		TIM4->EGR = TIM_EGR_UG;
		TIM4->CR1 |= TIM_CR1_CEN;
	}
}
#else
# error "don't know how to do zero cross detection!"
#endif

static enum hxb_error_code read_type(struct hxb_value* v)
{
	v->v_u8 = dimmer_type;
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write_type(const struct hxb_envelope* env)
{
	if (env->value.v_u8 > TYPE_PWM)
		return HXB_ERR_INVALID_VALUE;

	init_timer();
	return HXB_ERR_SUCCESS;
}

static const char ep_type[] RODATA = "Dimmer type";
static ENDPOINT_DESCRIPTOR endpoint_type = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_DIMMER_MODE,
	.name = ep_type,
	.read = read_type,
	.write = write_type
};

static enum hxb_error_code read_brightness(struct hxb_value* v)
{
	v->v_float = brightness_pct / 100.0f;
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write_brightness(const struct hxb_envelope* env)
{
	if (env->value.v_float < 0.0f || env->value.v_float > 1.0f)
		return HXB_ERR_INVALID_VALUE;

	brightness_pct = env->value.v_float * 100;
	update_timer();
	return HXB_ERR_SUCCESS;
}

static const char ep_brightness[] RODATA = "Dimmer brightness";
static ENDPOINT_DESCRIPTOR endpoint_brightness = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_DIMMER_BRIGHTNESS,
	.name = ep_brightness,
	.read = read_brightness,
	.write = write_brightness
};

void dimmer_init(void)
{
	init_pins();
	init_timer();

	ENDPOINT_REGISTER(endpoint_type);
	ENDPOINT_REGISTER(endpoint_brightness);
}
