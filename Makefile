LIBS := libnl-genl-3.0

CFLAGS := $(shell pkg-config --cflags $(LIBS))
LDFLAGS := $(shell pkg-config --libs $(LIBS))

rftool: rftool.o
	g++ $(LDFLAGS) -o $@ $<
	cp $@ ../$@

%.o: %.cpp
	g++ -c -o $@ $(CFLAGS) $<

i2c:

run:
	sudo modprobe -r at86rf230
	sudo modprobe at86rf230
	sudo /home/hexabus/rftool
