TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64

TARGETS_BUBBLE := $(addsuffix .elf, $(addprefix bubble_, $(TYPES)))
TARGETS_INSERTION := $(addsuffix .elf, $(addprefix insertion_, $(TYPES)))
TARGETS_SELECTION := $(addsuffix .elf, $(addprefix selection_, $(TYPES)))
TARGETS_MERGE := $(addsuffix .elf, $(addprefix merge_, $(TYPES)))
TARGETS_QUICK := $(addsuffix .elf, $(addprefix quick_, $(TYPES)))
TARGETS_HEAP := $(addsuffix .elf, $(addprefix heap_, $(TYPES)))

HEADERS := test_arrays.h
PYVER := python3
CODEGEN_PY := codegen.py

OPT := -O2
CFLAGS :=
export CFLAGS
TARGET :=
export TARGET

all: bubble insertion selection merge quick heap

bubble: $(TARGETS_BUBBLE)
insertion: $(TARGETS_INSERTION)
selection: $(TARGETS_SELECTION)
merge: $(TARGETS_MERGE)
quick: $(TARGETS_QUICK)
heap: $(TARGETS_HEAP)

headers: $(HEADERS)

%.elf: $(HEADERS) *.c *.h
	$(eval TARGET := $(basename $@))
	$(eval ALG := $(word 1, $(subst _, ,$(basename $@))))
	$(eval NF := $(word 2, $(subst _, ,$(basename $@))))
	$(eval CFLAGS := $(OPT) -DNF_$(shell echo $(NF) | tr '[:lower:]' '[:upper:]') -DALG_$(shell echo $(ALG) | tr '[:lower:]' '[:upper:]'))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f ../Makefile.inc --no-print-directory

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
