DHRY_ITERS = 1000000u
TARGET := dhrystone
GCC_OPTS += -O3
LIBS += -lc
COMMON_OBJ_NAMES = crt0.o common.o nanoprintf.o newlib_defs.o
CFLAGS += -DDHRY_ITERS=$(DHRY_ITERS)
CFLAGS += -D_USE_NEWLIB
CFLAGS += -DTIME -DNOENUM -Wno-implicit -fno-builtin-printf
CFLAGS += -fno-common -falign-functions=64 -fno-inline
CFLAGS += -mexplicit-relocs

all: $(TARGET).elf

include ../Makefile.inc
