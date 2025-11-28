#TARGET := n_$(N_IN)
LOOPS := 100
TYPES := load store mixed
TARGETS := $(addsuffix .elf, $(TYPES))

TEST_DEFINES := $(foreach t,$(TYPES), $(t):-D$(shell echo $(t) | tr a-z A-Z))

$(info $(TEST_DEFINES))

CACHE_ALIGNED := 1

ifeq ($(CACHE_ALIGNED), 1)
CFLAGS += -DCACHE_ALIGNED
endif

CFLAGS += -DLOOPS=$(LOOPS)

OPT := -O3 -flto

all: $(TARGETS)

include ../Makefile.inc
