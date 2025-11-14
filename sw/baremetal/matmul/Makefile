LOOP_ORDERS := ijk jik jki ikj kij kji
LOOPS := 1

TARGETS := $(addsuffix .elf, $(addprefix int8_, $(LOOP_ORDERS)))

TEST_DEFINES := $(foreach t,$(LOOP_ORDERS), \
				int8_$(t):-DLOOP_ORDER_$(shell echo $(t) | tr a-z A-Z))

OPT := -O1 -flto
CFLAGS += -DLOOPS=$(LOOPS)

HEADERS := test_matrices.h
PYVER := python3
CODEGEN_PY := codegen.py
INC_PATHS += .

all: int8

int8: $(TARGETS)

codegen: headers
clean_codegen: clean_headers

headers: $(HEADERS)

# dependent on the dcache size
A_ROWS = 32
B_COLS = 32
A_COLS_B_ROWS = 32

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) $(A_ROWS) $(B_COLS) $(A_COLS_B_ROWS)

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
