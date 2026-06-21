SHELL := /bin/bash

NPROCS_DEF := 8
NPROCS := $(shell nproc 2>/dev/null || echo $(NPROCS_DEF))
# only add -j if MAKEFLAGS doesn't already contain a job limit
ifeq ($(filter -j% --jobserver%, $(MAKEFLAGS)),)
    MAKEFLAGS += -j$(NPROCS)
endif

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
DEFINES += -DRV32C
endif

UART_IN ?= 0
ifeq ($(UART_IN), 1)
DEFINES += -DUART_INPUT_EN
endif

DEBUG ?= 0
ifeq ($(DEBUG), 1)
DEFINES += -DDEBUG
endif

CXXFLAGS := -Wall -Wextra
CXXFLAGS += -Wcast-qual -Wold-style-cast
CXXFLAGS += -Wunreachable-code -Wnull-dereference
CXXFLAGS += -Wnon-virtual-dtor -Woverloaded-virtual
CXXFLAGS += -Werror -pedantic -std=gnu++17
CXXFLAGS += -Wno-error=null-dereference # may be required for ELFIO only
CXXFLAGS += -O3 -flto=auto # release build
#CXXFLAGS += -g -fno-omit-frame-pointer # perf profiling
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
SOURCES := $(wildcard *.cpp)
SOURCES += $(wildcard devices/*.cpp)
SOURCES += $(wildcard profilers/*.cpp)
SOURCES += $(wildcard hw_models/*.cpp)
OBJECTS := $(patsubst %.cpp, $(BDIR)/%.o, $(SOURCES))
DEPS := $(patsubst %.cpp, $(BDIR)/%.d, $(SOURCES))
INC = -I. -I./devices -I./profilers -I./hw_models
INC += -I./$(BDIR)
INC += -I./external/cxxopts/include
INC += -isystem ./external/ELFIO # because it goes haywire on old-style-cast
INC += $(INC_EXT)

COSIM_OBJECTS := $(filter-out $(BDIR)/hw_models/%.o $(BDIR)/main.o, $(OBJECTS))

all: $(TARGET)

obj: $(OBJECTS)

obj_for_cosim: $(COSIM_OBJECTS)

include Makefile.autogen

NO_SYMLINK ?= 0 # to skip symlinking into the parent directory, e.g. for test build
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
	@if [ "$(NO_SYMLINK)" = "0" ]; then \
		ln -sf $@ $(BIN); \
	fi

strip: $(TARGET)
	$(STRIP) $(STRIPFLAGS) $(TARGET)

$(BDIR)/%.o: %.cpp | $(VER_H)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES) $(USER_DEFINES) $(INC)

-include $(DEPS)

clean:
	rm -rf $(BDIR)

.PHONY: all obj obj_for_cosim strip clean
