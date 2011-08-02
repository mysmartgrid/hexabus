CC=gcc
CFLAGS=-c -Wall -I. -std=c99 -pedantic -fPIC -O3 -D_POSIX_C_SOURCE=200112L
#LDFLAGS=-static
LDFLAGS=-lc
SERVER_SOURCES=6switchd.c #workload.cpp workload-factory.cpp job.cpp 
#SOURCES+=allocation.cpp reportwriter.cpp schedulearchive.cpp
SERVER_OBJECTS=$(SERVER_SOURCES:.c=.o)
SERVER_EXECUTABLE=6switchd
CLIENT_SOURCES=6switch.c #workload.cpp workload-factory.cpp job.cpp 
#SOURCES+=allocation.cpp reportwriter.cpp schedulearchive.cpp
CLIENT_OBJECTS=$(CLIENT_SOURCES:.c=.o)
CLIENT_EXECUTABLE=6switch

all: $(SERVER_SOURCES) $(SERVER_EXECUTABLE) $(CLIENT_SOURCES) $(CLIENT_EXECUTABLE)
	
$(SERVER_EXECUTABLE): $(SERVER_OBJECTS)
	$(CC) $(LDFLAGS) $(SERVER_OBJECTS) -o $@

$(CLIENT_EXECUTABLE): $(CLIENT_OBJECTS)
	$(CC) $(LDFLAGS) $(CLIENT_OBJECTS) -o $@

# GCC autodepend-fu
.c.o:
	$(CC) $(CFLAGS) -MD $< -o $@
	@sed 's,\($*\.o\)[ :]*\(.*\),$@ : $$\(wildcard \2\)\n\1 : \2,g' $*.d > /dev/null
	
#@sed -i 's,\($*\.o\)[ :]*\(.*\),$@ : $$\(wildcard \2\)\n\1 : \2,g' $*.d > /dev/null
	
.PHONY: clean
clean:
	rm -f $(SERVER_OBJECTS) $(SERVER_EXECUTABLE) $(CLIENT_OBJECTS) $(CLIENT_EXECUTABLE) *.d

-include $(SERVER_SOURCES:.c=.d)
-include $(CLIENT_SOURCES:.c=.d)
