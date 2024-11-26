LOOPS := 1
TYPES := int4 int8 int16 int16_int8 int16_int4 int8_int4
TARGETS := $(addsuffix .elf, $(TYPES))

#MATH_LIB_OPT ?= -DLOAD_OPT
MATH_LIB_OPT ?= -DCUSTOM_ISA
#MATH_LIB_OPT += -DFORCE_INLINE
#MATH_LIB_OPT += -DSTREAMING

OPT := -O3 -flto
CFLAGS := -DLOOPS=$(LOOPS) $(MATH_LIB_OPT) $(OPT)
export CFLAGS
TARGET :=
export TARGET

all: $(TARGETS)

%.elf: *.c
	$(eval TARGET := $(basename $@))
	$(eval CFLAGS := $(CFLAGS) -DNF_$(shell echo $(TARGET) | tr '[:lower:]' '[:upper:]'))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f ../Makefile.inc --no-print-directory

include ../Makefile.inc
