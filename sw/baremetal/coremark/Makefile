TARGET := coremark
ITERATIONS := 6500

GCC_OPTS += -O2 -flto
CFLAGS += -DFLAGS_STR="\"$(GCC_OPTS)\"" -DITERATIONS=${ITERATIONS} -DPERFORMANCE_RUN=1
GCC_OPTS += -Wall -Werror

all: $(TARGET).elf

.PHONY: check
check:
	md5sum -c coremark.md5

include ../Makefile.inc
