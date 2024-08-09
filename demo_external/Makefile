TOPDIR := /home/nvidia/data/J5/platform_sdk
SRC_HOST_DIR := /home/nvidia/data/J5/platform_sdk/prebuilts/host
TOOLCHAIN_NAME_64 := gcc-ubuntu-9.3.0-2020.03-x86_64-aarch64-linux-gnu
CFLAGS := -O2 -g
# CFLAGS += -fPIC -Wall -g
VPATH += ./src
TARGET = ./rtsps_demo_external
CROSS_COMPILE = aarch64-linux-gnu-g++
INC = -I./include
INC += -I./include/codec

SOURCES := $(foreach dir,$(VPATH),$(wildcard $(dir)/*))
SRCS = $(filter %.cpp,$(SOURCES))

LDFLAGS += -L./lib
LDFLAGS += -Wl,-rpath-link=./lib
LDFLAGS += -Wl,-rpath-link=${SRC_HOST_DIR}/${TOOLCHAIN_NAME_64}/aarch64-linux-gnu/lib
LIBS +=  -lmultimedia -lhbmem -lion -lalog -lpthread -ldl -lm -lvio -lvio_client -lcam -lvpm -lrtsps  -lhb_api_isp -lrtppay -ldiaglib
OBJS = $(SRCS:.cpp=.o)
%.o:%.cpp
	$(CROSS_COMPILE) $(CFLAGS) $(INC)  -o $@ -c $<

.PHONY: all all_32 clean

all all_32:$(TARGET)
$(TARGET): $(OBJS)
	$(CROSS_COMPILE) $(LDFLAGS) $(OBJS) $(LIBS)  -o $@
	rm -rf $(OBJS)

clean:
	rm -rf $(TARGET) $(OBJS)
