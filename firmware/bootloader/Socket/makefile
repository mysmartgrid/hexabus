CONTIKI = ../../contiki
APPDIR = ../../apps
PLATFORMDIR = ../../platform

HEXABUS_TARGET = Socket

include $(APPDIR)/nvm/Makefile.nvm

AVRMCU = atmega1284p
F_CPU = 8000000
HEADERS = Bootloader.h
SRC += Bootloader.c \
			nullnetwork.c \
			nvm/$(nvm_src) \
			cpu/avr/radio/rf230bb/halbb.c \
			cpu/avr/radio/rf230bb/rf230bb.c \
			core/net/rime/rimeaddr.c \
			core/net/mac/framer-802154.c \
			core/net/mac/frame802154.c \
			core/net/packetbuf.c \
			core/lib/random.c \
			core/sys/process.c \
			core/net/queuebuf.c \
			core/lib/memb.c \
			core/net/mac/nullrdc.c \
			core/net/mac/nullmac.c \
			core/net/mac/mac.c \


OUTPUT = hexabus-bootloader
OBJDIR = bootloader_bin

VPATH = .:$(APPDIR):$(CONTIKI)

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
OBJ = $(SRC:%.c=$(OBJDIR)/%.o)
CFLAGS = -Os -Wall -Wstrict-prototypes -std=gnu99
CFLAGS += -fshort-enums -fpack-struct -funsigned-char -funsigned-bitfields -ffunction-sections -fdata-sections
CFLAGS += -mmcu=$(AVRMCU) -DF_CPU=$(F_CPU)UL -DVERSION=$(VERSION) -DRF230BB=1 -DRF230_IS_212=1
CFLAGS += -I. -I $(APPDIR)/nvm/ -I $(CONTIKI)/core/ -I $(CONTIKI)/cpu/avr/ -I $(CONTIKI)/cpu/avr/radio/rf230bb/ -I $(PLATFORMDIR)/Hexabus-Socket/
LDFLAGS += -Wl,-Ttext=0xF800,--gc-sections,--defsym=_text=0xF800

all: $(OBJDIR)/$(OUTPUT).hex size
	@echo ":: Bootloader done !"

$(OBJDIR)/%.o : %.c $(HEADERS)
	@mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/$(OUTPUT).elf : $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,--start-group $+ -Wl,--end-group -o $@

$(OBJDIR)/$(OUTPUT).hex : $(OBJDIR)/$(OUTPUT).elf
	$(OBJCOPY) -O ihex $< $@

size : $(OBJDIR)/$(OUTPUT).elf
	@echo
	@$(SIZE) --mcu=$(AVRMCU) -C $(OBJDIR)/$(OUTPUT).elf
	@echo

clean :
	@rm -rf $(OBJDIR)