
RV_GNU := riscv64-unknown-elf
SHELL  := /bin/bash

C_SRCS := $(wildcard *.c)
S_SRCS := $(wildcard *.S)
LD_SRC := ../link.ld
SRCS := $(C_SRCS) $(S_SRCS) $(CRT0)
INC := ../inc
USER_DEFINES =

GCC_OPTS += -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -ffreestanding -nostdlib -nostartfiles -T $(LD_SRC) -Wl,--build-id=none -save-temps
#GCC_OPTS += g

all: $(TARGET).elf

# run with RTL_SIM=1 to generate hex file for RTL simulation
$(TARGET).elf: $(SRCS)
	@$(RV_GNU)-gcc $(GCC_OPTS) -I$(INC) $^ -o $@ $(USER_DEFINES) -lgcc
	@$(RV_GNU)-objdump -D -Mnumeric -t --disassembler-options=no-aliases $@ > $(basename $@).dump
	@$(RV_GNU)-objcopy $(basename $@).elf -O binary $(basename $@).bin
	@if [ -n "$(RTL_SIM)" -a "$(RTL_SIM)" == "1" ]; then \
		$(RV_GNU)-bin2hex -w 32 $(basename $@).bin $(basename $@).hex; \
	fi
	@echo "Generated $@"

clean:
	rm -f *.elf *.dump *.hex *.bin *.o *.d *.i *.s

.PHONY: target
