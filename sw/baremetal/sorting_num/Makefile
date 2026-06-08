#TYPES := uint8 int8 uint16 int16 uint32 int32 uint64 int64 float32 float64
TYPES := int8 int16 uint32 int32 int64 float32
#LENGTHS := tiny small medium large
LENGTHS := small large
ALGS := bubble insertion selection merge quick heap quicker
LOOPS := 1

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py --groups "$(ALGS):ALG" "$(TYPES):NF" "$(LENGTHS):LEN" > $@

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
