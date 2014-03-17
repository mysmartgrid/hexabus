rftool:
	g++ `pkg-config --libs libnl-genl-3.0` `pkg-config --cflags libnl-genl-3.0` -o../rftool rftool.cpp

i2c:

run:
	sudo modprobe -r at86rf230
	sudo modprobe at86rf230
	sudo /home/hexabus/rftool
