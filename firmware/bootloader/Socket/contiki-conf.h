#ifndef iy7reiutrioytriu6r5
#define iy7reiutrioytriu6r5

#include_next "contiki-conf.h"

#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID 0xFFFF // Broadcast PANID

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC nullrdc_driver
#undef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK nullnetwork_driver

#endif