LOOPS := 1
TYPES := int16 # int4 int8 int16 int16_int8 int16_int4 int8_int4 int8_int2
LENGTHS := small large
TARGETS := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(TYPES)))

TEST_DEFINES := $(foreach t,$(TYPES), \
				$(foreach l,$(LENGTHS), \
				$(t)_$(l):-DNF_$(shell echo $(t) | tr a-z A-Z):-DLEN_$(shell echo $(l) | tr a-z A-Z)))

#MATH_LIB_FLAGS ?= -DLOAD_OPT
MATH_LIB_FLAGS ?= -DCUSTOM_ISA
#MATH_LIB_FLAGS += -DFORCE_INLINE
#MATH_LIB_FLAGS += -DSTREAMING

OPT := -O2 -flto
CFLAGS := -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)
CFLAGS += -DALIGNED

all: $(TARGETS)

include ../Makefile.inc
