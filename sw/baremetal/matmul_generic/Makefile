LOOP_ORDERS := mnk nmk nkm mkn kmn knm
LOOPS := 1

NF := int8

-include parametrize.mk
parametrize.mk: ../parametrize.py
	python3 ../parametrize.py "$(NF):-" "$(LOOP_ORDERS):LOOP_ORDER" > $@

OPT := -O1 -flto
CFLAGS += -DLOOPS=$(LOOPS)

HEADERS := test_matrices.h
CODEGEN_PY := ./codegen.py
INC_PATHS += .

all: int8

int8: $(TARGETS)

codegen: headers
clean_codegen: clean_headers

headers: $(HEADERS)

# dependent on the dcache size, data shouldn't fit in dcache
M = 64
N = 64
K = 64

$(HEADERS): $(CODEGEN_PY)
	$(CODEGEN_PY) $(NF) $(M) $(N) $(K)

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
