#!/bin/sh

avrdude -c jtag2 -P usb -p atmega1284p -U eeprom:r:temp.eep:r -F
head -c 14 temp.eep > temp.head
# select power meter calibration value from eeprom
tail -c 4033 temp.eep | head -c 3 > cal_value
cat temp.head plug_plus.middle cal_value plug_plus.tail > temp_full.eep

echo 1
# give the jtagice some time to reinitalize and connect to PC and target
sleep 3

echo 2
avrdude -c jtag2 -P usb -F -p atmega1284p -U hfuse:w:0x98:m
sleep 3

echo 3
avrdude -c jtag2 -P usb -F -p atmega1284p -e
sleep 3

echo 4
avrdude -c jtag2 -P usb -p atmega1284p -U eeprom:w:temp_full.eep:r -F
sleep 3
echo 5
avrdude -c jtag2 -P usb -F -p atmega1284p -U hfuse:w:0x90:m
sleep 3
echo 6
avrdude -c jtag2 -P usb -p atmega1284p -U flash:w:plug_plus.bin:r -F
