SHELL := /bin/bash

include ../baremetal/Makefile.setup.inc

LD_SRC := link.ld

MARCH ?= rv32i_zicsr_zifencei
MABI ?= ilp32
GCC_OPTS += -march=$(MARCH) -mabi=$(MABI) -mcmodel=medany -static -fno-pic
GCC_OPTS += -nostdlib -nostartfiles -ffreestanding -g
GCC_OPTS += -ffunction-sections -fdata-sections -msmall-data-limit=8
#GCC_OPTS += -fvisibility=hidden
#GCC_OPTS += -save-temps
GCC_OPTS += -T $(LD_SRC) -Wl,--build-id=none
GCC_OPTS += -Wl,--gc-sections
GCC_OPTS += -Wl,--no-warn-rwx-segments
#GCC_OPTS += -Wl,--noinhibit-exec
#GCC_OPTS += -Wl,-Map=$(TARGET).map

DIR ?= riscv-tests/isa/rv32ui

OBJDUMP_OPTS := -Dhxtw --show-all-symbols
OBJDUMP_OPTS += --start-address=0x80000000 --stop-address=0x80020000
OBJDUMP_OPTS += -Mnumeric -Mno-aliases # rv specific
#OBJDUMP_OPTS += --visualize-jumps
#OBJDUMP_OPTS += -S # inline C source code in disassembly

HEX ?= 1
MEM_WIDTH := 128

TESTS := $(notdir $(shell find $(DIR) -type f -name "*.S"))
TESTS_BIN := $(subst .S,.elf,$(TESTS))
# remove "ma_data" from the list of tests if it exists (rv32ui only)
TESTS_BIN := $(filter-out %ma_data.elf,$(TESTS_BIN))

all: $(TESTS_BIN)

%.elf: $(DIR)/%.S
	@$(GCC) $(GCC_OPTS) -Idefines -Iriscv-tests/env -Iriscv-tests/isa/macros/scalar $^ -o $(basename $@).elf
	@$(OBJDUMP) $(OBJDUMP_OPTS) $(basename $@).elf > $(basename $@).dasm
	@if [ "$(HEX)" = "1" ]; then \
		$(OBJCOPY) $(basename $@).elf -O binary $(basename $@).bin; \
		$(BIN2HEX) -w $(MEM_WIDTH) -a 0 $(basename $@).bin $(basename $@).mem; \
	fi
	@echo "Executable built: $@"
	@$(SIZE) -G $@

clean:
	rm -f *.elf* *.dasm *.hex* *.mem* *.bin *.o *.d *.i *.s *.map
