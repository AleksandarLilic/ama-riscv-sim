RV_GNU := riscv64-linux-gnu
LD_SRC := link.ld
GCC_OPTS := -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -T $(LD_SRC) -Wl,--build-id=none

# override from cli if needed
DIR ?= riscv-tests/isa/rv32ui

TESTS := $(notdir $(shell find $(DIR) -type f -name "*.S"))
TESTS_BIN := $(subst .S,.bin,$(TESTS))
# remove "ma_data" from the list of tests if it exists
TESTS_BIN := $(filter-out %ma_data.bin,$(TESTS_BIN))

all: $(TESTS_BIN)

%.bin: $(DIR)/%.S
	$(RV_GNU)-gcc $(GCC_OPTS) -Idefines -Iriscv-tests/env -Iriscv-tests/isa/macros/scalar $^ -o $(basename $@).elf
	$(RV_GNU)-objdump -D -Mnumeric --disassembler-options=no-aliases $(basename $@).elf > $(basename $@).dump
	$(RV_GNU)-objcopy $(basename $@).elf -O binary $(basename $@).bin
	$(RV_GNU)-bin2hex -w 32 $(basename $@).bin $(basename $@).hex

clean:
	rm -f *.elf *.bin *.hex *.dump
