TARGET := ustress
LOOPS ?= 400
CFLAGS += -DLOOPS=$(LOOPS)

LIBS += -lc
COMMON_OBJ_NAMES = crt0.o common.o nanoprintf.o newlib_defs.o
CFLAGS += -D_USE_NEWLIB

include ../Makefile.ustress.inc
