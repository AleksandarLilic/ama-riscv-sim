
RV_GNU := riscv64-unknown-elf
SHELL  := /bin/bash

C_SRCS := $(wildcard *.c)
S_SRCS := $(wildcard *.S)
LD_SRC := ../link.ld
SRCS := $(C_SRCS) $(S_SRCS) $(CRT0)
INC := ../defines
USER_DEFINES =

GCC_OPTS += -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -ffreestanding -nostdlib -nostartfiles -T $(LD_SRC) -Wl,--build-id=none -save-temps
#GCC_OPTS += g

all: $(TARGET).elf

$(TARGET).elf: $(SRCS)
	$(RV_GNU)-gcc $(GCC_OPTS) -I$(INC) $^ -o $@ $(USER_DEFINES) -lgcc
	$(RV_GNU)-objdump -D -Mnumeric -t --disassembler-options=no-aliases $@ > $(basename $@).dump
	$(RV_GNU)-objcopy $(basename $@).elf -O binary $(basename $@).bin
	$(RV_GNU)-bin2hex -w 32 $(basename $@).bin $(basename $@).hex

clean:
	rm -f *.elf *.dump *.hex *.bin *.o *.d *.i *.s

.PHONY: target
