LOOP_ORDERS := ijk jik jki ikj kij kji
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

# dependent on the dcache size
A_ROWS = 64
B_COLS = 64
A_COLS_B_ROWS = 64

$(HEADERS): $(CODEGEN_PY)
	$(CODEGEN_PY) $(NF) $(A_ROWS) $(B_COLS) $(A_COLS_B_ROWS)

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
