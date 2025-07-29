RV_GNU := $(RV_GNU_LATEST)
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
OBJDUMP_OPTS += --start-address=0x40000 --stop-address=0x50000
OBJDUMP_OPTS += -Mnumeric -Mno-aliases # rv specific
#OBJDUMP_OPTS += --visualize-jumps
#OBJDUMP_OPTS += -S # inline C source code in disassembly
MEM_WIDTH := 32

TESTS := $(notdir $(shell find $(DIR) -type f -name "*.S"))
TESTS_BIN := $(subst .S,.bin,$(TESTS))
# remove "ma_data" from the list of tests if it exists (rv32ui only)
TESTS_BIN := $(filter-out %ma_data.bin,$(TESTS_BIN))

RV_GNU_DEFAULT := riscv64-unknown-elf
BIN2HEX :=
HEX ?= 1
ifeq ($(HEX), 1)
	ifneq (, $(shell command -v $(RV_GNU)-bin2hex 2> /dev/null))
		BIN2HEX := $(RV_GNU)-bin2hex
	else ifneq (, $(shell command -v $(RV_GNU_DEFAULT)-bin2hex 2> /dev/null))
		BIN2HEX := $(RV_GNU_DEFAULT)-bin2hex
	else
		$(error "No bin2hex tool found")
	endif
endif

all: $(TESTS_BIN)

%.bin: $(DIR)/%.S
	@$(RV_GNU)-gcc $(GCC_OPTS) -Idefines -Iriscv-tests/env -Iriscv-tests/isa/macros/scalar $^ -o $(basename $@).elf
	@$(RV_GNU)-objdump $(OBJDUMP_OPTS) $(basename $@).elf > $(basename $@).dump
	@$(RV_GNU)-objcopy $(basename $@).elf -O binary $(basename $@).bin
	@if [ "$(HEX)" = "1" ]; then \
		$(BIN2HEX) -w $(MEM_WIDTH) $(basename $@).bin $(basename $@).hex; \
	fi
	@echo "Generated $@"

clean:
	rm -f *.elf* *.dump *.hex *.bin *.o *.d *.i *.s *_symbols.json *.map
