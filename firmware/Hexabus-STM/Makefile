all: main.bin
	 arm-none-eabi-size main.Hexabus-STM

#APPS =hexabus-webserver udp_handler temperature button endpoint_registry
#APPS +=datetime_service shutter relay state_machine button_handlers hexapush 
#APPS +=metering shutter relay provisioning value_broadcast hxb_broadcast_handler
#APPS +=presence_detector hexonoff analogread i2c humidity pressure 
#APPS +=ir_receiver  memory_debugger sm_upload epaper pt100 hexasense
UIP_CONF_IPV6=1
APPS = nvm

CONTIKI = ../contiki
TARGETDIRS = ../platform
APPDIRS += ../apps/

HEXABUS_TARGET = STM

include $(CONTIKI)/Makefile.include

CFLAGS += -Wall -Wno-unused -Wswitch-enum -std=gnu99 -I../../shared
CFLAGS += -Werror

CFLAGS += -D'__SHORT_FILE__="$(notdir $<)"'
