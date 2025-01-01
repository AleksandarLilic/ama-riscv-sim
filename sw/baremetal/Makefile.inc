
REPO_ROOT := $(shell git rev-parse --show-toplevel)

# set up for gcc14
RV_GNU := $(RV_GNU_LATEST)
# e.g. `export RV_GNU_LATEST=riscv64-unknown-elf`
SHELL  := /bin/bash

# if not set in test's Makefile, default to all object files
COMMON_OBJ_NAMES ?= crt0.o common.o common_math.o mini-printf.o
COMMON_SRC_PATH := $(REPO_ROOT)/sw/baremetal/common
COMMON_BUILD_DIR := ./common_build
COMMON_OBJS = $(addprefix $(COMMON_BUILD_DIR)/, $(COMMON_OBJ_NAMES))
$(shell mkdir -p $(COMMON_BUILD_DIR))
vpath %.c $(COMMON_SRC_PATH)
vpath %.h $(COMMON_SRC_PATH)
vpath %.S $(COMMON_SRC_PATH)

C_SRCS := $(wildcard *.c)
S_SRCS := $(wildcard *.S)
LD_SRC := $(COMMON_SRC_PATH)/link.ld
SRCS := $(C_SRCS) $(S_SRCS)
INC += $(COMMON_SRC_PATH)
INCS := $(wildcard $(addsuffix /*.h, $(INC)))
LIBS += -lgcc
USER_DEFINES =

# if not set in test's Makefile, default to all supported ISA extensions
#MARCH ?= rv32imc_zicsr_zifencei_zicntr
MARCH ?= rv32i_zmmul_zicsr_zifencei_zicntr
#MARCH ?= rv32i_zicsr
MABI ?= ilp32
GCC_OPTS += -march=$(MARCH) -mabi=${MABI} -mcmodel=medany
GCC_OPTS += -static -fno-pic
GCC_OPTS += -fvisibility=hidden -ffreestanding -nostdlib -nostartfiles -g
#GCC_OPTS += -save-temps
GCC_OPTS += -T $(LD_SRC) -Wl,--build-id=none
GCC_OPTS += -Wl,--no-warn-rwx-segments
#GCC_OPTS += -Wl,-Map=$(TARGET).map
# optimizations not set - expected to come from test's Makefile

OBJDUMP_OPTS := -D -Mnumeric -t --disassembler-options=no-aliases
#OBJDUMP_OPTS := -S # inline C source code in disassembly
MEM_WIDTH := 32

# for tests that are parameterized by defines
define GET_DEFINES
$(wordlist 2,999,$(subst :, ,$(filter $1:%,$(TEST_DEFINES))))
endef

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

# run with HEX=1 to generate hex file for RTL simulation
%.elf: $(SRCS) $(INCS) $(CRT0) $(LD_SRC) $(COMMON_OBJS) $(APP_REQS)
	@$(RV_GNU)-gcc $(GCC_OPTS) $(OPT) $(CFLAGS) $(call GET_DEFINES,$*) -I$(INC) $(SRCS) $(COMMON_OBJS) -o $@ $(USER_DEFINES) $(LIBS)
	@$(RV_GNU)-objdump $(OBJDUMP_OPTS) $@ > $(basename $@).dump
	@$(RV_GNU)-objcopy $@ -O binary $(basename $@).bin
	@if [ "$(HEX)" = "1" ]; then \
		$(BIN2HEX) -w $(MEM_WIDTH) $(basename $@).bin $(basename $@).hex; \
	fi
	@echo "Generated $@"
	@$(RV_GNU)-size $@

build_common: $(COMMON_OBJS)

COMPILE_COMMON = @$(RV_GNU)-gcc $(GCC_OPTS) $(OPT) $(CFLAGS) -c $< -o $@ $(USER_DEFINES)

$(COMMON_BUILD_DIR)/%.o: %.c %.h
	$(COMPILE_COMMON)

$(COMMON_BUILD_DIR)/%.o: %.c
	$(COMPILE_COMMON)

$(COMMON_BUILD_DIR)/%.o: %.S
	$(COMPILE_COMMON)

clean:
	rm -f *.elf* *.dump *.hex *.bin *.o *.d *.i *.s *_symbols.json *.map

clean_common:
	rm -rf $(COMMON_BUILD_DIR)

cleanall: clean clean_common

.PHONY: all build_common clean clean_common cleanall
