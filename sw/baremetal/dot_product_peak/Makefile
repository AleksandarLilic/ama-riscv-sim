LOOPS := 10
TYPES := single_acc multiple_acc
SUFFIX := _int8
TARGETS := $(addsuffix $(SUFFIX).elf, $(TYPES))
TEST_DEFINES := $(foreach t,$(TYPES), $(t)$(SUFFIX):-D$(shell echo $(t) | tr a-z A-Z))

OPT := -O3 -flto
CFLAGS := -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)

$(info $(TARGETS))
$(info $(TEST_DEFINES))

all: $(TARGETS)

include ../Makefile.inc
