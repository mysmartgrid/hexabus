#include "hexabus_app_bootstrap.h"
#include "platform_init.h"

#include <string.h>

#include "button_handlers.h"
#include "nvm.h"

#include "contiki.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/uip.h"
#include "rf230bb.h"
#include "health.h"

#if WEBSERVER
#include "httpd-fs.h"
#endif

#define ANNOUNCE_BOOT 1
#if ANNOUNCE_BOOT
#define PRINTF(fmt, ...) printf_rofmt(ROSTR(fmt), ##__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define DEBUG 1
#if DEBUG
#define PRINTFD(fmt, ...) printf_rofmt(ROSTR(fmt), ##__VA_ARGS__)
#else
#define PRINTFD(...)
#endif

uint8_t encryption_enabled = 1;

void log_message(const char *m1, const char *m2)
{
  PRINTFD("%s%s\n", m1, m2);
}

static void set_device_addrs()
{
	extern uint16_t mac_dst_pan_id;
	extern uint16_t mac_src_pan_id;

	rimeaddr_t addr = {};
	nvm_read_block(mac_addr, addr.u8, nvm_size(mac_addr));

	memcpy(&uip_lladdr.addr, addr.u8, 8);
	rf230_set_pan_addr(nvm_read_u16(pan_id), nvm_read_u16(pan_addr), addr.u8);
	rf230_set_channel(RF_CHANNEL);
 
	mac_dst_pan_id = nvm_read_u16(pan_id);
	mac_src_pan_id = mac_dst_pan_id;

	rimeaddr_set_node_addr(&addr);

	PRINTFD("MAC address ");
	for (int i = 0; i < 8; i++) {
		if (i)
			PRINTFD(":");
		PRINTFD("%02x", addr.u8[i]);
	}
	PRINTFD("\n");
}

static void init_crypto()
{
	uint8_t key[16];

	nvm_read_block(encryption_key, key, 16);
	rf230_key_setup(key);
}

static void initialize()
{
	platform_init();
	PRINTF("\n*******Booting starting initialisation.\n");

	watchdog_init();
	watchdog_start();

	clock_init();

	button_handlers_init();

	leds_on(LEDS_ALL);
	clock_delay_usec(50000);
	watchdog_periodic();
	leds_off(LEDS_ALL);

	PRINTF("\n*******Booting %s*******\n", CONTIKI_VERSION_STRING);

	process_init();
	process_start(&etimer_process, NULL);
	ctimer_init();

	queuebuf_init();
	netstack_init();

	set_device_addrs();
	init_crypto();

	PRINTF("mac:%s rdc:%s, channel %u", NETSTACK_MAC.name, NETSTACK_RDC.name, rf230_get_channel());
	if (NETSTACK_RDC.channel_check_interval && NETSTACK_RDC.channel_check_interval()) {
		PRINTF(", check rate %u Hz", (unsigned) (CLOCK_SECOND / NETSTACK_RDC.channel_check_interval()));
	}
	PRINTF("\n");

#if UIP_CONF_IPV6_RPL
	PRINTF("RPL Enabled\n");
#endif
#if UIP_CONF_ROUTER
	PRINTF("Routing Enabled\n");
#endif
	PRINTF("process_start\n");

	process_start(&tcpip_process, NULL);
	watchdog_periodic();

	PRINTF("rf230_generate_random_byte\n");
	random_init((rf230_generate_random_byte() << 8) | rf230_generate_random_byte());
	watchdog_periodic();
  
	PRINTF("app_bootstrap\n");
	hexabus_app_bootstrap();
	watchdog_periodic();
	PRINTF("app_bootstrap - done\n");
	{
		char buf[nvm_size(domain_name) + 1] = {};
		nvm_read_block(domain_name, buf, nvm_size(domain_name));

#if WEBSERVER
		unsigned size = httpd_fs_get_size();
		PRINTF("%s online with %u byte web content\n", buf, size);
#else
		PRINTF("%s online\n", buf);
#endif
	}
	watchdog_periodic();
	PRINTF("initialization done\n");
}

int main()
{
	initialize();

	while(1) {
		process_run();
		watchdog_periodic();
	}
	return 0;
}
