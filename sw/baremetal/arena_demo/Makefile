TARGET := arena

# libc needed for assert.h only
LIBS += -lc
COMMON_OBJ_NAMES = crt0.o memset.o common.o common_math.o mini-printf.o newlib_defs.o

MATH_LIB_FLAGS ?= -DCUSTOM_ISA
CFLAGS +=  $(MATH_LIB_FLAGS)
CFLAGS += -D_USE_NEWLIB
#CFLAGS += -DARENA_ASSERT=

GCC_OPTS += -O2 -flto

all: $(TARGET).elf

include ../Makefile.inc
