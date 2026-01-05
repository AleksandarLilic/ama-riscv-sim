LOOPS := 1
TYPES := int4 int8 int16 int16_int8 int16_int4 int8_int4 int8_int2
TARGETS := $(addsuffix .elf, $(TYPES))

TEST_DEFINES := $(foreach t,$(TYPES), $(t):-DNF_$(shell echo $(t) | tr a-z A-Z))

#MATH_LIB_FLAGS ?= -DLOAD_OPT
MATH_LIB_FLAGS ?= -DCUSTOM_ISA
MATH_LIB_FLAGS += -DFORCE_INLINE
MATH_LIB_FLAGS += -DSIMD_UNROLL

OPT := -O3 -flto
CFLAGS := -DLOOPS=$(LOOPS) $(MATH_LIB_FLAGS)

all: $(TARGETS)

include ../Makefile.inc
