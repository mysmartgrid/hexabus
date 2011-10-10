MY_NAME := hexaswitch
MY_SRCS := $(wildcard *.cpp)
MY_OBJS := ${MY_SRCS:.cpp=.o}
MY_INCLUDE_DIRS :=
MY_LIBRARY_DIRS :=
MY_LIBRARIES := boost_system

CPPFLAGS += $(foreach includedir,$(MY_INCLUDE_DIRS),-I$(includedir))
CPPFLAGS += -Wall -g
LDFLAGS += $(foreach librarydir,$(MY_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(MY_LIBRARIES),-l$(library))

.PHONY: all clean

all: ${MY_NAME}

$(MY_NAME): $(MY_OBJS)
	$(LINK.cc) $(MY_OBJS) -o $(MY_NAME)

clean:
	@- rm -rf $(MY_OBJS) $(MY_NAME)
