TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64
OPS := add sub mul div
# Combine operations with types
FLAVORS := $(foreach op,$(OPS),$(addprefix $(op)_,$(TYPES)))
TARGETS := $(addsuffix .elf, $(FLAVORS))
HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(OPS)))
PYVER := python3
CODEGEN_PY := codegen.py

OPT := -O2
CFLAGS :=
export CFLAGS
TARGET :=
export TARGET

all: $(TARGETS)

headers: $(HEADERS)

%.elf: $(HEADERS)
	$(eval TARGET := $(basename $@))
	$(eval OP := $(word 1, $(subst _, ,$(basename $@))))
	$(eval NF := $(word 2, $(subst _, ,$(basename $@))))
	$(eval CFLAGS := $(OPT) -DNF_$(shell echo $(NF) | tr '[:lower:]' '[:upper:]') -DOP_$(shell echo $(OP) | tr '[:lower:]' '[:upper:]'))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f ../Makefile.inc --no-print-directory

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) --op $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
