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

init-jtag2:
	@[ -n "${NAME}" ] || (echo "NAME required"; exit 1)
	@[ `echo -n "${NAME}" | wc -c` -lt 31 ] || (echo "NAME too long"; exit 1)
	( \
		echo erase; \
		echo "write lfuse 0 0xe2"; \
		echo "write hfuse 0 0x10"; \
		echo "write efuse 0 0xff"; \
		echo "write eeprom 1 0x02 0xb5 0x5a 0xff 0xfe 0x0b ${SHORTADDR}"; \
		i=33; for j in `echo -n '${NAME}' | hexdump -e '/1 " 0x%.2x"'`; do echo write eeprom $$i $$j; i=$$(($$i+1)); done; \
		while [ $$i -lt 63 ]; do echo write eeprom $$i 0; i=$$(($$i + 1)); done; \
	) | avrdude -p atmega1284p -P usb -c jtag2 -F -t

calinit:
	avrdude -p atmega1284p -P usb -c jtag2 -F -U eeprom:r:sock.eep:r -U hfuse:w:0x98:m
	(echo -n "Plug"; dd if=sock.eep bs=1 skip=7 count=2 status=noxfer | hexdump -e '/1 "%.2x"') \
		| dd of=sock.eep seek=33 bs=1 status=noxfer conv=notrunc
	dd if=/dev/zero of=sock.eep bs=1 seek=41 count=2 conv=notrunc status=noxfer
	/bin/echo -en '\x00\x00\x1c\x00\xff' | dd of=sock.eep bs=1 seek=63 count=5 conv=notrunc status=noxfer
	sleep 5
	avrdude -p atmega1284p -P usb -c jtag2 -F -e -U eeprom:w:sock.eep:r -U hfuse:w:0x90:m

flash-jtag2: 
	avrdude -c jtag2 -P usb -p atmega1284p -U flash:w:Hexabus-Socket.hex -F 

flash-jtag3: 
	avrdude -c jtag3 -P usb -p atmega1284p -U flash:w:Hexabus-Socket.hex -F 

flash-dragon: 
	avrdude -c dragon_jtag -P usb -p atmega1284p -U flash:w:Hexabus-Socket.hex -F 
