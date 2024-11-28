LOOPS := 1
TYPES := int4 int8 int16 int16_int8 int16_int4 int8_int4
TARGETS := $(addsuffix .elf, $(TYPES))

#MATH_LIB_OPT ?= -DLOAD_OPT
MATH_LIB_OPT ?= -DCUSTOM_ISA
#MATH_LIB_OPT += -DFORCE_INLINE
#MATH_LIB_OPT += -DSTREAMING
export MATH_LIB_OPT

OPT := -O3 -flto
FLAGS := -DLOOPS=$(LOOPS) $(OPT) $(MATH_LIB_OPT)
CFLAGS :=
export CFLAGS
TARGET :=
export TARGET

all: $(TARGETS)

%.elf: *.c
	$(eval TARGET := $(basename $@))
	$(eval CFLAGS := $(FLAGS) -DNF_$(shell echo $(TARGET) | tr '[:lower:]' '[:upper:]'))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f ../Makefile.inc --no-print-directory

include ../Makefile.inc
