#TARGET := n_$(N_IN)
TARGET := basic
CACHE_ALIGNED := 1

ifeq ($(CACHE_ALIGNED), 1)
CFLAGS += -DCACHE_ALIGNED
endif

OPT := -O3 -flto

all: $(TARGET).elf

include ../Makefile.inc
