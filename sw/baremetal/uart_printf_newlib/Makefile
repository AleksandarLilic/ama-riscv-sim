TARGET := hello_world
GCC_OPTS += -O3 -flto
LIBS += -lc
COMMON_OBJ_NAMES = crt0.o common.o newlib_defs.o
CFLAGS += -D_USE_NEWLIB -DFORCE_NEWLIB_PRINTF

all: $(TARGET).elf

include ../Makefile.inc
