SHELL := /bin/bash

NPROCS_DEF := 8
NPROCS := $(shell nproc 2>/dev/null || echo $(NPROCS_DEF))
# only add -j if MAKEFLAGS doesn't already contain a job limit
ifeq ($(filter -j% --jobserver%, $(MAKEFLAGS)),)
    MAKEFLAGS += -j$(NPROCS)
endif

REPO_ROOT := $(shell git rev-parse --show-toplevel)

RV_GNU := $(RV_GNU_LATEST)
# e.g. `export RV_GNU_LATEST=riscv64-unknown-elf`
BIN2HEX := $(REPO_ROOT)/sw/bin2hex.py

# Turn on increased build verbosity by defining BUILD_VERBOSE in test Makefile
# or use V=1 on the make command line
BUILD_VERBOSE ?= 0

ifeq ("$(origin V)", "command line")
BUILD_VERBOSE := $(V)
endif

ifeq ($(BUILD_VERBOSE), 0)
Q = @
else
Q =
endif

# if not set in test's Makefile, default to all object files w/o newlib
COMMON_OBJ_NAMES ?= crt0.o memset.o common.o common_math.o nanoprintf.o # newlib_defs.o
COMMON_SRC_PATH := $(REPO_ROOT)/sw/baremetal/common
COMMON_BUILD_DIR := ./common_build
COMMON_OBJS = $(addprefix $(COMMON_BUILD_DIR)/, $(COMMON_OBJ_NAMES))
$(shell mkdir -p $(COMMON_BUILD_DIR))
vpath %.c $(COMMON_SRC_PATH)
vpath %.h $(COMMON_SRC_PATH)
vpath %.S $(COMMON_SRC_PATH)

ifneq ($(EXPLICIT_SRCS), 1)
C_SRCS += $(wildcard *.c)
S_SRCS += $(wildcard *.S)
CPP_SRCS += $(wildcard *.cpp)
endif
LD_SRC := $(COMMON_SRC_PATH)/link.ld
SRCS := $(C_SRCS) $(S_SRCS) $(CPP_SRCS)
INC_PATHS += $(COMMON_SRC_PATH) .
INC_H := $(wildcard $(addsuffix /*.h, $(INC_PATHS)))
INC := $(addprefix -I, $(INC_PATHS))
LIBS += -lgcc
USER_DEFINES ?=

# if not set in test's Makefile, default to all supported ISA extensions in RTL
MARCH ?= rv32im_zicsr_zifencei_zicntr_xsimd
MABI ?= ilp32
GCC_OPTS += -march=$(MARCH) -mabi=$(MABI) -mcmodel=medany
GCC_OPTS += -mstrict-align -mcsr-check
GCC_OPTS += -static -fno-pic
GCC_OPTS += -nostdlib -nostartfiles -ffreestanding
GCC_OPTS += -g

# aligning code and data
CACHE_LINE_BYTES := 64
GCC_OPTS += -ffunction-sections -fdata-sections -msmall-data-limit=8
#GCC_OPTS += -fstack-usage
#GCC_OPTS += -specs=nano.specs
#GCC_OPTS += -falign-functions=$(CACHE_LINE_BYTES)
#GCC_OPTS += -falign-loops=$(CACHE_LINE_BYTES)
#GCC_OPTS += -falign-jumps=$(CACHE_LINE_BYTES)
#GCC_OPTS += -falign-labels=$(CACHE_LINE_BYTES)

#GCC_OPTS += -fvisibility=hidden
#GCC_OPTS += -save-temps
GCC_OPTS += -T $(LD_SRC) -Wl,--build-id=none
GCC_OPTS += -Wl,--gc-sections
GCC_OPTS += -Wl,--no-warn-rwx-segments
#GCC_OPTS += -Wa,-march=rv32i_zicsr_xsimd
#GCC_OPTS += -Wp,-w # disable preprocessor warnings
#GCC_OPTS += -Wl,--noinhibit-exec
#GCC_OPTS += -Wl,-Map=$(TARGET).map
# optimizations not set - expected to come from test's Makefile

CXX_OPTS += -fno-exceptions -fno-rtti
CXX_OPTS += -fno-threadsafe-statics -fno-use-cxa-atexit

OBJDUMP_OPTS := -Dhxtw --show-all-symbols
OBJDUMP_OPTS += --start-address=0x80000000 --stop-address=0x80020000
OBJDUMP_OPTS += -Mnumeric -Mno-aliases # rv specific
#OBJDUMP_OPTS += -z # show zero padding as well (complete dump)
#OBJDUMP_OPTS += --visualize-jumps

OD_SRC ?= 0
ifeq ($(OD_SRC), 1)
OBJDUMP_OPTS += -S # inline source code in disassembly
endif

HEX ?= 1
MEM_WIDTH := 128

ifeq ($(CPP_SRCS),)
    CC_DRIVER := $(RV_GNU)-gcc
    CXX_FLAGS :=
else
    CC_DRIVER := $(RV_GNU)-g++
    CXX_FLAGS := $(CXX_OPTS)
endif

# define as variable so it's not broken up across lines for stdout
BUILD_APP = \
    $(CC_DRIVER) $(GCC_OPTS) $(CXX_FLAGS) $(OPT) $(CFLAGS) $(USER_DEFINES) \
		$(EXTRA_DEFINES) $(INC) $(SRCS) $(COMMON_OBJS) -o $@ $(LIBS)

HEX_GEN = \
	if [ "$(HEX)" = "1" ]; then \
		$(RV_GNU)-objcopy $@ -O binary $(basename $@).bin; \
		$(BIN2HEX) -w $(MEM_WIDTH) -a 0 $(basename $@).bin $(basename $@).mem; \
	fi

%.elf: $(SRCS) $(INC_H) $(CRT0) $(LD_SRC) $(COMMON_OBJS) $(APP_REQS)
	$(Q)$(BUILD_APP)
	$(Q)$(RV_GNU)-objdump $(OBJDUMP_OPTS) $@ > $(basename $@).dasm
	$(Q)$(HEX_GEN)
	@echo "Executable built: $@"
	$(Q)$(RV_GNU)-size -G $@

build_common: $(COMMON_OBJS)

COMMON_FLAGS := -Wall -Wextra -Wunreachable-code -Wnull-dereference -Wcast-qual
COMMON_FLAGS += -Werror -pedantic

BUILD_COMMON = \
	$(Q)$(RV_GNU)-gcc $(GCC_OPTS) $(COMMON_FLAGS) $(OPT) $(CFLAGS) \
		$(USER_DEFINES) -MMD -MP -c $< -o $@

$(COMMON_BUILD_DIR)/%.o: %.c
	$(BUILD_COMMON)

$(COMMON_BUILD_DIR)/%.o: %.S
	$(BUILD_COMMON)

-include $(wildcard $(COMMON_BUILD_DIR)/*.d)

clean:
	rm -f *.elf* *.dasm *.hex* *.mem* *.bin *.o *.d *.i *.s *.map *.su

clean_common:
	rm -rf $(COMMON_BUILD_DIR)

cleanall: clean clean_common
	rm -rf parametrize.mk

.PHONY: all build_common clean clean_common cleanall
