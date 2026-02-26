
# optimization variants
OPT_V := load_b tiling txp_b
LOOPS := 1

TARGETS := $(addsuffix .elf, $(addprefix int8_, $(OPT_V)))

TEST_DEFINES := $(foreach t,$(OPT_V), \
				int8_$(t):-DOPT_V_$(shell echo $(t) | tr a-z A-Z))

OPT := -O3 -flto
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
A_ROWS = 64
B_COLS = 64
A_COLS_B_ROWS = 64

$(HEADERS): $(CODEGEN_PY)
	$(PYVER) $(CODEGEN_PY) $(A_ROWS) $(B_COLS) $(A_COLS_B_ROWS)

clean_headers:
	rm -f $(HEADERS)

include ../Makefile.inc
