DHRY_ITERS = 1000000u
TARGET := dhrystone
GCC_OPTS += -O3
LIBS += -lc
USE_NEWLIB := 1
CFLAGS += -DDHRY_ITERS=$(DHRY_ITERS)
CFLAGS += -DTIME -DNOENUM -Wno-implicit -fno-builtin-printf
CFLAGS += -fno-common -falign-functions=64 -fno-inline
CFLAGS += -mexplicit-relocs

all: $(TARGET).elf

include ../Makefile.inc
