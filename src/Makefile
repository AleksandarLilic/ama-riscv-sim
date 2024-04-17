CXX := g++
CXXFLAGS := -Wall -Wextra -Werror -pedantic -g -std=gnu++17 -O3 -save-temps # -pg
TARGET := ama-riscv-sim
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(SOURCES:.cpp=.d)
USER_DEFINES =
DEFINES =
#DEFINES += -DLOG_EXEC
#DEFINES += -DENABLE_DASM
#DEFINES += -DUSE_ABI_NAMES
#DEFINES += -DENABLE_PROF
#DEFINES += -DCHECK_LOG
DEFINES += -DUART_ENABLE
DEFINES += -DUART_INPUT_ENABLE

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES) $(USER_DEFINES)

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS) *.log *inst_profiler.json *.i *.s *.d *.ii

.PHONY: all clean
