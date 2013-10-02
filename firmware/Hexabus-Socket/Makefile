all: ipso
	 mv ipso.Hexabus-Socket Hexabus-Socket.elf
	 avr-objcopy -O ihex -R .signature -R .fuse -R .eeprom Hexabus-Socket.elf Hexabus-Socket.hex
	 avr-size -C --mcu=atmega1284P Hexabus-Socket.elf
#	 avr-size -C --mcu=atmega2561 Hexabus-Socket.elf
#	 avr-size -C --mcu=atxmega256d3 Hexabus-Socket.elf
APPS =hexabus-webserver udp_handler temperature button endpoint_registry
APPS +=datetime_service shutter relay state_machine button_handlers hexapush 
APPS +=metering shutter relay provisioning value_broadcast hxb_broadcast_handler
APPS +=presence_detector hexonoff analogread i2c humidity pressure 
APPS +=ir_receiver  memory_debugger sm_upload epaper pt100 hexasense
UIP_CONF_IPV6=1

CONTIKI=../contiki-2.x
#CONTIKI=../contiki-2.4
include $(CONTIKI)/Makefile.include

CFLAGS += -Wall -Werror -Wno-unused -Wswitch-enum

CFLAGS += -D"__SHORT_FILE__=\"`basename $<`\""

flash-jtag2: 
	avrdude -c jtag2 -P usb -p atmega1284p -U flash:w:Hexabus-Socket.hex -F 

flash-jtag3: 
	avrdude -c jtag3 -P usb -p atmega1284p -U flash:w:Hexabus-Socket.hex -F 

flash-dragon: 
	avrdude -c dragon_jtag -P usb -p atmega1284p -U flash:w:Hexabus-Socket.hex -F 
