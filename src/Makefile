CXX := g++
CXXFLAGS := -Wall -Wextra
CXXFLAGS += -Wcast-qual -Wold-style-cast
CXXFLAGS += -Wunreachable-code -Wnull-dereference
CXXFLAGS += -Wnon-virtual-dtor -Woverloaded-virtual
CXXFLAGS += -Werror -pedantic -std=gnu++17
CXXFLAGS += -Ofast -s -flto=auto -march=native -mtune=native # release build
#CXXFLAGS += -O0 -g # -fsanitize=address -fsanitize=undefined # gdb build
#CXXFLAGS += -save-temps
LDFLAGS :=

BDIR := build
$(shell mkdir -p $(BDIR))

TARGET := ama-riscv-sim
SOURCES := $(wildcard *.cpp)
SOURCES += $(wildcard profilers/*.cpp)
SOURCES += $(wildcard hw_models/*.cpp)
OBJECTS := $(patsubst %.cpp, $(BDIR)/%.o, $(SOURCES))
DEPS := $(patsubst %.cpp, $(BDIR)/%.d, $(SOURCES))
INC := -I. -I./profilers -I./hw_models -I./external/cxxopts/include

USER_DEFINES =
DEFINES := -DENABLE_DASM
#DEFINES += -DRV32C
#DEFINES += -DUART_INPUT_ENABLE

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(BDIR)/$@ $^ $(LDFLAGS)
	@ln -sf $(BDIR)/$@ $@

$(BDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES) $(USER_DEFINES) $(INC)

-include $(DEPS)

cleanlogs:
	rm -rf out_*

clean: cleanlogs
	rm -rf $(BDIR) $(TARGET)

.PHONY: all clean cleanlogs
