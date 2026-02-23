LOOPS := 1
TARGET := int8

OPT := -O1 -flto
CFLAGS += -DLOOPS=$(LOOPS)

HEADERS := test_matrices.h
PYVER := python3
CODEGEN_PY := codegen.py
INC_PATHS += .

all: $(TARGET).elf

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
