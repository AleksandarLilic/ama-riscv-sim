CXX := g++
CXXFLAGS := -Wall -Wextra
CXXFLAGS += -Wcast-qual -Wold-style-cast
CXXFLAGS += -Wunreachable-code -Wnull-dereference
CXXFLAGS += -Wnon-virtual-dtor -Woverloaded-virtual
CXXFLAGS += -Werror -pedantic -std=gnu++17
CXXFLAGS += -Wno-error=null-dereference # may be required for ELFIO only
CXXFLAGS += -Ofast -flto=auto # release build
#CXXFLAGS += -g -fno-omit-frame-pointer # perf profiling, but drop -s below
#CXXFLAGS += -O0 -g -fsanitize=address -fsanitize=undefined # gdb build
#CXXFLAGS += -save-temps
#CXXFLAGS += -ftime-report

TUNE ?= native
ifeq ($(TUNE), generic)
CXXFLAGS += -march=x86-64 -mtune=generic
else ifeq ($(TUNE), native)
CXXFLAGS += -march=native -mtune=native
endif

LDFLAGS := -s

STATIC_LINKING ?= 0 # default: dynamic linking
ifeq ($(STATIC_LINKING), 1) # static linking of C++ runtime only
LDFLAGS += -static-libstdc++ -static-libgcc
else ifeq ($(STATIC_LINKING), 2) # full static linking
LDFLAGS += -static
endif

LDLIBS :=

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

USER_DEFINES =
DEFINES := -DDASM_EN
#DEFINES += -DDEBUG
#DEFINES += -DRV32C
#DEFINES += -DUART_INPUT_EN

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

$(BDIR)/%.o: %.cpp | $(VER_H)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES) $(USER_DEFINES) $(INC)

-include $(DEPS)

cleanbuild:
	rm -rf $(BDIR)

clean: cleanlogs cleanbuild
	rm -rf $(TARGET)

.PHONY: all obj clean cleanlogs cleanbuild
