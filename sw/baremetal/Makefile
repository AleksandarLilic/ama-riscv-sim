
RV_GNU := riscv64-linux-gnu
SHELL  := /bin/bash

C_SRCS := $(wildcard *.c)
S_SRCS := $(wildcard *.S)
LD_SRC := link.ld
LIB_PATH := ../libgcc_wannabe
LIB_SRCS := $(wildcard $(LIB_PATH)/*.c)
SRCS := $(C_SRCS) $(S_SRCS) $(LIB_SRCS)
INC := ../defines

GCC_OPTS += -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -T $(LD_SRC) -Wl,--build-id=none -save-temps

all: $(TARGET).elf

$(TARGET).elf: $(SRCS)
	$(RV_GNU)-gcc $(GCC_OPTS) -I$(INC) $^ -o $@
	$(RV_GNU)-objdump -D -Mnumeric --disassembler-options=no-aliases $@ > $(basename $@).dump
	$(RV_GNU)-objcopy $(basename $@).elf -O binary $(basename $@).bin
	$(RV_GNU)-bin2hex -w 32 $(basename $@).bin $(basename $@).hex

clean:
	rm -f *.elf *.dump *.hex *.bin *.o *.d *.i *.s

.PHONY: target
