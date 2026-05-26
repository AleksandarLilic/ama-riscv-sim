TARGET := ustress
CFLAGS += -DLOOPS=1000

LIBS += -lc
COMMON_OBJ_NAMES = crt0.o common.o nanoprintf.o newlib_defs.o
CFLAGS += -D_USE_NEWLIB

include ../Makefile.ustress.inc
