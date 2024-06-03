TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64
OPS := add sub mul div

TARGETS_ADD := $(addsuffix .elf, $(addprefix $(word 1, $(OPS))_, $(TYPES)))
TARGETS_SUB := $(addsuffix .elf, $(addprefix $(word 2, $(OPS))_, $(TYPES)))
TARGETS_MUL := $(addsuffix .elf, $(addprefix $(word 3, $(OPS))_, $(TYPES)))
TARGETS_DIV := $(addsuffix .elf, $(addprefix $(word 4, $(OPS))_, $(TYPES)))

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(OPS)))
PYVER := python3
CODEGEN_PY := codegen.py

OPT := -O2
CFLAGS :=
export CFLAGS
TARGET :=
export TARGET

all: add sub mul div

add: $(TARGETS_ADD)
sub: $(TARGETS_SUB)
mul: $(TARGETS_MUL)
div: $(TARGETS_DIV)

headers: $(HEADERS)

%.elf: $(HEADERS) *.c *.h
	$(eval TARGET := $(basename $@))
	$(eval OP := $(word 1, $(subst _, ,$(basename $@))))
	$(eval NF := $(word 2, $(subst _, ,$(basename $@))))
	$(eval CFLAGS := $(OPT) -DNF_$(shell echo $(NF) | tr '[:lower:]' '[:upper:]') -DOP_$(shell echo $(OP) | tr '[:lower:]' '[:upper:]'))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f ../Makefile.inc --no-print-directory

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
