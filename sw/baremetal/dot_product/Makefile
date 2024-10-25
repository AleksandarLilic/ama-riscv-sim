LOOPS := 1
TYPES := int4 int8 int16
TARGETS := $(addsuffix .elf, $(TYPES))

CUSTOM_ISA ?= -DCUSTOM_ISA
#CUSTOM_ISA ?= -DLOAD_OPT
OPT := -O3 -flto
FLAGS += -DLOOPS=$(LOOPS) $(CUSTOM_ISA)
CFLAGS :=
export CFLAGS
TARGET :=
export TARGET

all: $(TARGETS)

%.elf: *.c
	$(eval TARGET := $(basename $@))
	$(eval NF := $(word 1, $(subst _, ,$(basename $@))))
	$(eval CFLAGS := $(OPT) $(FLAGS) -DNF_$(shell echo $(NF) | tr '[:lower:]' '[:upper:]'))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f ../Makefile.inc --no-print-directory

include ../Makefile.inc
