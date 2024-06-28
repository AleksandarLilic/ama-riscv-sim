TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64
#LENGTHS := tiny small medium large
LENGTHS := small large

TARGETS_BUBBLE := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix bubble_, $(TYPES))))
TARGETS_INSERTION := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix insertion_, $(TYPES))))
TARGETS_SELECTION := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix selection_, $(TYPES))))
TARGETS_MERGE := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix merge_, $(TYPES))))
TARGETS_QUICK := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix quick_, $(TYPES))))
TARGETS_HEAP := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix heap_, $(TYPES))))

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(LENGTHS)))
PYVER := python3
CODEGEN_PY := codegen.py

OPT := -O3 -flto
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
	$(eval LEN := $(word 3, $(subst _, ,$(basename $@))))
	$(eval CFLAGS := $(OPT) -DNF_$(shell echo $(NF) | tr '[:lower:]' '[:upper:]') -DALG_$(shell echo $(ALG) | tr '[:lower:]' '[:upper:]') -DLEN_$(shell echo $(LEN) | tr '[:lower:]' '[:upper:]'))
	@echo Building $@ with $(CFLAGS)
	@$(MAKE) -f ../Makefile.inc --no-print-directory

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
