CXX := g++
CXXFLAGS := -Wall -Wextra
CXXFLAGS += -Wcast-qual -Wold-style-cast
CXXFLAGS += -Wunreachable-code -Wnull-dereference
CXXFLAGS += -Wnon-virtual-dtor -Woverloaded-virtual
CXXFLAGS += -Werror -pedantic -std=gnu++17
CXXFLAGS += -Wno-error=null-dereference # may be required for ELFIO only
CXXFLAGS += -Ofast -s -flto=auto -march=native -mtune=native # release build
#CXXFLAGS += -g -fno-omit-frame-pointer # for perf profiling, but drop -s above
#CXXFLAGS += -O0 -g # -fsanitize=address -fsanitize=undefined # gdb build
#CXXFLAGS += -save-temps
LDFLAGS :=

BDIR ?= build
$(shell mkdir -p $(BDIR))

TARGET := ama-riscv-sim
SOURCES := $(wildcard *.cpp)
SOURCES += $(wildcard devices/*.cpp)
SOURCES += $(wildcard profilers/*.cpp)
SOURCES += $(wildcard hw_models/*.cpp)
OBJECTS := $(patsubst %.cpp, $(BDIR)/%.o, $(SOURCES))
DEPS := $(patsubst %.cpp, $(BDIR)/%.d, $(SOURCES))
INC := -I. -I./devices -I./profilers -I./hw_models
INC += -I./external/cxxopts/include
INC += -isystem ./external/ELFIO # because it goes haywire on old-style-cast

USER_DEFINES =
DEFINES := -DDASM_EN
#DEFINES += -DRV32C
#DEFINES += -DUART_INPUT_EN

all: $(TARGET)

obj: $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(BDIR)/$@ $^ $(LDFLAGS)
	@ln -sf $(BDIR)/$@ $@

$(BDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES) $(USER_DEFINES) $(INC)

-include $(DEPS)

cleanlogs:
	rm -rf out_*

cleanbuild:
	rm -rf $(BDIR)

clean: cleanlogs cleanbuild
	rm -rf $(TARGET)

.PHONY: all obj clean cleanlogs cleanbuild
