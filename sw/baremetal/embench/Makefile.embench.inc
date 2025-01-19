GCC_OPTS += -O2 -flto
INC_PATHS += ../support
C_SRCS += ../support/main.c ../support/beebsc.c

LIBS += -lc -lm
COMMON_OBJ_NAMES = crt0.o common.o mini-printf.o newlib_defs.o
CFLAGS += -D_USE_NEWLIB

CPU_MHZ ?= 100
CFLAGS += -DCPU_MHZ=$(CPU_MHZ) -DWARMUP_HEAT=10

all: $(TARGET).elf

include ../../Makefile.inc
