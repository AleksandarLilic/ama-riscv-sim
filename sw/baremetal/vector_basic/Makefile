TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64
OPS := add_ sub_ mul_ div_
# Combine operations with types
FLAVORS := $(foreach op,$(OPS),$(addprefix $(op),$(TYPES)))
TARGETS := $(addsuffix .elf, $(FLAVORS))

OPT := -O2
CFLAGS :=
export CFLAGS
TARGET := 
export TARGET
CRT0 := ../crt0.S
export CRT0

# Default target
all: $(TARGETS)

# Generic pattern rule for all variants
# FIXME: doesn't work with -j since main makefile generates main.o and crt0.o for all targets
%.elf:
	$(eval TARGET := $(basename $@))
	$(eval OP := $(word 1, $(subst _, ,$(basename $@))))
	$(eval NF := $(word 2, $(subst _, ,$(basename $@))))
	$(eval CFLAGS := $(OPT) -DNF_$(shell echo $(NF) | tr '[:lower:]' '[:upper:]') -DOP_$(shell echo $(OP) | tr '[:lower:]' '[:upper:]'))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f ../Makefile --no-print-directory

include ../Makefile
