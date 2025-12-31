TARGET := ustress
CFLAGS += -DLOOPS=100

LIBS += -lc
COMMON_OBJ_NAMES = crt0.o common.o mini-printf.o newlib_defs.o
CFLAGS += -D_USE_NEWLIB

include ../Makefile.ustress.inc
