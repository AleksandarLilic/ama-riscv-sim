TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64
#LENGTHS := tiny small medium large
LENGTHS := small large
ALGS := bubble insertion selection merge quick heap

TARGETS_BUBBLE := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix $(word 1, $(ALGS))_, $(TYPES))))
TARGETS_INSERTION := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix $(word 2, $(ALGS))_, $(TYPES))))
TARGETS_SELECTION := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix $(word 3, $(ALGS))_, $(TYPES))))
TARGETS_MERGE := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix $(word 4, $(ALGS))_, $(TYPES))))
TARGETS_QUICK := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix $(word 5, $(ALGS))_, $(TYPES))))
TARGETS_HEAP := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(addprefix $(word 6, $(ALGS))_, $(TYPES))))

TEST_DEFINES := $(foreach t,$(TYPES), \
                $(foreach l,$(LENGTHS), \
				$(foreach a,$(ALGS), \
                $(a)_$(t)_$(l):-DNF_$(shell echo $(t) | tr a-z A-Z):-DLEN_$(shell echo $(l) | tr a-z A-Z):-DALG_$(shell echo $(a) | tr a-z A-Z))))

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(LENGTHS)))
PYVER := python3
CODEGEN_PY := codegen.py

OPT := -O3 -flto
CFLAGS := -DLOOPS=$(LOOPS)

all: bubble insertion selection merge quick heap

bubble: $(TARGETS_BUBBLE)
insertion: $(TARGETS_INSERTION)
selection: $(TARGETS_SELECTION)
merge: $(TARGETS_MERGE)
quick: $(TARGETS_QUICK)
heap: $(TARGETS_HEAP)

headers: $(HEADERS)

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
