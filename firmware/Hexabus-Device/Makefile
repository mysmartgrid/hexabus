UIP_CONF_IPV6=1

APPS = \
	button \
	button_handlers \
	datetime_service \
	endpoint_registry \
	health \
	nvm \
	provisioning \
	sm_upload \
	state_machine \
	udp_handler \
	value_broadcast

CONTIKI = ../contiki
TARGETDIRS = ../platform
APPDIRS += ../apps

include $(CONTIKI)/Makefile.include

CFLAGS += -Wall -Werror -Wno-unused -Wswitch-enum -std=gnu99 -I../../shared

CFLAGS += -D'__SHORT_FILE__="$(notdir $<)"'
