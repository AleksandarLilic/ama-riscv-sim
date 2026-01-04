GCC_OPTS += -O2
INC_PATHS += ../support
C_SRCS += ../support/main.c ../support/beebsc.c

LIBS += -lm -lc
COMMON_OBJ_NAMES = crt0.o common.o mini-printf.o newlib_defs.o
CFLAGS += -D_USE_NEWLIB

CPU_MHZ ?= 100
WARMUP ?= 10
CFLAGS += -DCPU_MHZ=$(CPU_MHZ) -DWARMUP_HEAT=$(WARMUP)

all: $(TARGET).elf

include ../../Makefile.inc
