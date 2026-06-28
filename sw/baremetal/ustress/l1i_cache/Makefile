TARGET := ustress
LOOPS ?= 1000
CFLAGS += -DLOOPS=$(LOOPS)

LIBS += -lc
COMMON_OBJ_NAMES = crt0.o common.o nanoprintf.o newlib_defs.o
CFLAGS += -D_USE_NEWLIB

GCC_OPTS += -Wl,--defsym=__stack_size=1K

include ../Makefile.ustress.inc
