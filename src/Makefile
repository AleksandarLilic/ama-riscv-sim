SHELL := /bin/bash

NPROCS_DEF := 8
NPROCS := $(shell nproc 2>/dev/null || echo $(NPROCS_DEF))
# only add -j if MAKEFLAGS doesn't already contain a job limit
ifeq ($(filter -j% --jobserver%, $(MAKEFLAGS)),)
    MAKEFLAGS += -j$(NPROCS)
endif

MAKEFLAGS += --output-sync=line

CXX ?= g++

CXX_REQ := 10 # 9 may also work, but 10+ is preferred for gnu++17 support
CXX_VERSION := $(shell $(CXX) -dumpversion | cut -d. -f1)
ifeq ($(shell [ $(CXX_VERSION) -ge $(CXX_REQ) ] && echo ok),)
$(error "GCC >= $(CXX_REQ) required")
endif

USER_DEFINES ?=
DEFINES :=

PROFILERS ?= 1
ifeq ($(PROFILERS), 1)
DEFINES += -DPROFILERS_EN
endif

HW_MODELS ?= 1
ifeq ($(HW_MODELS), 1)
DEFINES += -DHW_MODELS_EN
endif

DASM ?= 0
ifeq ($(DASM), 1)
DEFINES += -DDASM_EN
endif

RV32C ?= 0
ifeq ($(RV32C), 1)
DEFINES += -DRV32C_EN
endif

SIMD ?= 1
ifeq ($(SIMD), 1)
DEFINES += -DSIMD_EN
endif

UART_IN ?= 0
ifeq ($(UART_IN), 1)
DEFINES += -DUART_INPUT_EN
endif

DEBUG ?= 0
ifeq ($(DEBUG), 1)
DEFINES += -DDEBUG
endif

DPI ?= 0
ifeq ($(DPI), 1)
DEFINES += -DDPI
endif

CACHE_MODE ?= 0
ifeq ($(CACHE_MODE), 1)
DEFINES += -DCACHE_MODE=CACHE_MODE_FUNC
endif

CACHE_VERIFY ?= 0
ifeq ($(CACHE_VERIFY), 1)
DEFINES += -DCACHE_VERIFY
endif

CXXFLAGS := -Wall -Wextra
CXXFLAGS += -Wcast-qual -Wold-style-cast
CXXFLAGS += -Wunreachable-code -Wnull-dereference
CXXFLAGS += -Wnon-virtual-dtor -Woverloaded-virtual
CXXFLAGS += -Werror -pedantic -std=gnu++17
CXXFLAGS += -Wno-error=null-dereference # may be required for ELFIO only
CXXFLAGS += -O3 -flto=auto # release build

PERF ?= 0 # perf/flamegraph profiling: debug symbols + frame pointers, keeps -O3
ifeq ($(PERF), 1)
CXXFLAGS += -g -fno-omit-frame-pointer
endif

#CXXFLAGS += -O0 -g -fsanitize=address -fsanitize=undefined # gdb build
#CXXFLAGS += -save-temps
#CXXFLAGS += -ftime-report

TUNE ?= native_x86
ifeq ($(TUNE), generic_x86)
CXXFLAGS += -march=x86-64 -mtune=generic
else ifeq ($(TUNE), native_x86)
CXXFLAGS += -march=native -mtune=native
else ifeq ($(TUNE), generic_arm)
CXXFLAGS += -march=armv8.5-a -mtune=generic
else ifeq ($(TUNE), native_arm)
CXXFLAGS += -mcpu=native
endif

LDFLAGS :=

STATIC_LINKING ?= 0 # default: dynamic linking
ifeq ($(STATIC_LINKING), 1) # static linking of C++ runtime only
LDFLAGS += -static-libstdc++ -static-libgcc
else ifeq ($(STATIC_LINKING), 2) # full static linking
LDFLAGS += -static
endif

LDLIBS :=

# post-link strip
# optional 'STRIPFLAGS=--strip-all' with GNU strip
STRIP ?= strip
STRIPFLAGS ?=

BDIR ?= build
$(shell mkdir -p $(BDIR))

BIN := ama-riscv-sim
TARGET := $(BDIR)/$(BIN)

ISA_SIM_SRC_ROOT := .
ISA_SIM_OBJ_ROOT := $(BDIR)
include Makefile.isa_sim_sources.mk

SOURCES := $(ISA_SIM_SRCS)
OBJECTS := $(ISA_SIM_OBJS)
DEPS := $(ISA_SIM_DEPS)
INC = $(ISA_SIM_INC)
INC += $(INC_EXTRA)

COSIM_OBJECTS := $(ISA_SIM_COSIM_OBJS)

all: $(TARGET)

obj: $(OBJECTS)

obj_for_cosim: $(COSIM_OBJECTS)

include Makefile.autogen.mk

V ?= 0
ifeq ($(V),1)
Q :=
MSG := @:
else
Q := @
MSG := @printf
endif

$(TARGET): $(OBJECTS)
	$(MSG) "  LD      $@\n"
	$(Q)$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

strip: $(TARGET)
	$(STRIP) $(STRIPFLAGS) $(TARGET)

$(BDIR)/%.o: %.cpp | $(VER_H)
	@mkdir -p $(dir $@)
	$(MSG) "  CXX     $<\n"
	$(Q)$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES) $(USER_DEFINES) $(INC)

-include $(DEPS)

clean:
	rm -rf $(BDIR)

.PHONY: all obj obj_for_cosim strip clean
