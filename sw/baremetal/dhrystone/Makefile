DHRY_ITERS = 8000000u
TARGET := dhrystone
GCC_OPTS += -O3
LIBS += -lc
COMMON_OBJ_NAMES = crt0.o common.o common_math.o mini-printf.o newlib_defs.o
CFLAGS += -DDHRY_ITERS=$(DHRY_ITERS)
CFLAGS += -D_USE_NEWLIB
CFLAGS += -DTIME -DNOENUM -Wno-implicit -fno-builtin-printf
CFLAGS += -fno-common -falign-functions=4 -fno-inline
include ../Makefile.inc
