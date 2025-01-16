#LENGTHS := tiny small medium large
LENGTHS := small large
ALGS := bubble insertion selection merge quick heap quicker
LOOPS := 1

TARGETS_BUBBLE := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(word 1, $(ALGS))))
TARGETS_INSERTION := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(word 2, $(ALGS))))
TARGETS_SELECTION := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(word 3, $(ALGS))))
TARGETS_MERGE := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(word 4, $(ALGS))))
TARGETS_QUICK := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(word 5, $(ALGS))))
TARGETS_HEAP := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(word 6, $(ALGS))))
TARGETS_QUICKER := $(foreach len, $(LENGTHS), $(addsuffix _$(len).elf, $(word 7, $(ALGS))))

TEST_DEFINES := $(foreach l,$(LENGTHS), \
				$(foreach a,$(ALGS), \
                $(a)_$(l):-DLEN_$(shell echo $(l) | tr a-z A-Z):-DALG_$(shell echo $(a) | tr a-z A-Z)))

HEADER_PREFIX := test_arrays_
HEADERS := $(addsuffix .h, $(addprefix $(HEADER_PREFIX), $(LENGTHS)))
PYVER := python3
CODEGEN_PY := codegen.py

OPT := -O3 -flto
LIBS += -lc
CFLAGS := -DLOOPS=$(LOOPS)

all: bubble insertion selection merge quick heap quicker

codegen: headers
clean_codegen: clean_headers

bubble: $(TARGETS_BUBBLE)
insertion: $(TARGETS_INSERTION)
selection: $(TARGETS_SELECTION)
merge: $(TARGETS_MERGE)
quick: $(TARGETS_QUICK)
heap: $(TARGETS_HEAP)
quicker: $(TARGETS_QUICKER)

headers: $(HEADERS)

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) $(word 3, $(subst _, ,$*))

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
