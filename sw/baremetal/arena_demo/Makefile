TARGET := arena

# libc needed for assert.h only
LIBS += -lc
USE_NEWLIB := 1

CFLAGS += $(MATH_LIB_FLAGS)
#CFLAGS += -DARENA_ASSERT=

GCC_OPTS += -O2 -flto

all: $(TARGET).elf

include ../Makefile.inc
