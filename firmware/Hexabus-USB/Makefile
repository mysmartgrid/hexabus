all: ipso
	 mv ipso.Hexabus-USB Hexabus-USB.elf
	 avr-objcopy -O ihex -R .signature -R .fuse -R .eeprom Hexabus-USB.elf Hexabus-USB.hex
	 avr-size -C --mcu=at90usb1287 Hexabus-USB.elf
	 
APPS= button bootloader provisioning
#UIP_CONF_IPV6=1
CONTIKI_NO_NET=1

CONTIKI=../contiki-2.x
#CONTIKI=../contiki-2.4
include $(CONTIKI)/Makefile.include



