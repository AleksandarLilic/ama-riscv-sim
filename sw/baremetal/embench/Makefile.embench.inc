OPT ?= -O2

GCC_OPTS += $(OPT)
INC_PATHS += ../support
C_SRCS += ../support/main.c ../support/beebsc.c

LIBS += -lm -lc
USE_NEWLIB := 1

CPU_MHZ ?= 100
WARMUP ?= 10
CFLAGS += -DCPU_MHZ=$(CPU_MHZ) -DWARMUP_HEAT=$(WARMUP)
CFLAGS += -DEMBENCH_TEST_NAME='"$(notdir $(CURDIR))"'

all: $(TARGET).elf

include ../../Makefile.inc
