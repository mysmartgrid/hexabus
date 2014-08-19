#include "stm32l1xx.h"
#include "contiki-hexabus-lowlevel.h"
#include "default-uart.h"
#include <stdio.h>
#include <string.h>

#include "sys/clock.h"
#include "sys/etimer.h"
#include "sys/process.h"
#include "dev/watchdog.h"
#include "rf230bb.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/uip.h"

#include "nvm.h"
#include "button.h"
#include "button_handlers.h"
#include "udp_handler.h"

uint8_t encryption_enabled = 1;

void get_aes128key_from_eeprom(char* foo)
{
	nvm_read_block(encryption_key, foo, nvm_size(encryption_key));
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

	queuebuf_init();
	netstack_init();
	rimeaddr_t addr = { { 0x02, 0x54, 0, 0xff, 0xfe, 1, 2, 3 } };

	memcpy(&uip_lladdr.addr, addr.u8, 8);
	rf230_set_pan_addr(nvm_read_u16(pan_id), 0, addr.u8);
	rf230_set_channel(RF_CHANNEL);
	extern uint16_t mac_dst_pan_id;
	extern uint16_t mac_src_pan_id;
	//set pan_id for frame creation
	mac_dst_pan_id = nvm_read_u16(pan_id);
	mac_src_pan_id = mac_dst_pan_id;
	rimeaddr_set_node_addr(&addr);
	process_start(&etimer_process, NULL);
	process_start(&tcpip_process, NULL);

	button_handlers_init();
	process_start(&etimer_process, 0);
	process_start(&button_pressed_process, 0);
	process_start(&udp_handler_process, 0);

	for (unsigned i = 0;; i++) {
		process_run();
		watchdog_periodic();
	}

	while (1);
}
