TARGET := hello_world
GCC_OPTS += -O3 -flto
LIBS += -lc

USE_NEWLIB := 1
CFLAGS += -DFORCE_NEWLIB_PRINTF

all: $(TARGET).elf

include ../Makefile.inc
