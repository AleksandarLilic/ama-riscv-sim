RV_GNU := riscv64-linux-gnu

LD_SRC := link.ld

GCC_OPTS := -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -T $(LD_SRC) -Wl,--build-id=none

TESTS := $(notdir $(shell find riscv-tests/isa/rv32ui -type f -name "*.S"))
TESTS_BIN := $(subst .S,.bin,$(TESTS))
# remove "fence_i" and "ma_data" from the list of tests
TESTS_BIN := $(filter-out %fence_i.bin %ma_data.bin,$(TESTS_BIN))

all: $(TESTS_BIN)

%.bin: riscv-tests/isa/rv32ui/%.S
	@echo "$<"
	$(RV_GNU)-gcc $(GCC_OPTS) -Idefines -Iriscv-tests/env -Iriscv-tests/isa/macros/scalar $^ -o $(basename $@).elf
	$(RV_GNU)-objdump -D -Mnumeric --disassembler-options=no-aliases $(basename $@).elf > $(basename $@).dump
	$(RV_GNU)-objcopy $(basename $@).elf -O binary $(basename $@).bin
	$(RV_GNU)-bin2hex -w 32 $(basename $@).bin $(basename $@).hex

clean:
	rm -f *.elf *.bin *.hex *.dump