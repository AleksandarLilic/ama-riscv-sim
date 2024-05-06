CXX := g++
CXXFLAGS := -Wall -Wextra -Werror -pedantic -std=gnu++17 -save-temps
CXXFLAGS += -O3 -s
#CXXFLAGS += -pg
TARGET := ama-riscv-sim
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(SOURCES:.cpp=.d)
USER_DEFINES =
DEFINES =
#DEFINES += -DLOG_EXEC
#DEFINES += -DENABLE_DASM
#DEFINES += -DUSE_ABI_NAMES
DEFINES += -DENABLE_PROF
#DEFINES += -DCHECK_LOG
DEFINES += -DUART_ENABLE
#DEFINES += -DUART_INPUT_ENABLE

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES) $(USER_DEFINES)

-include $(DEPS)

cleanlogs:
	rm -f *.log *_profiler.* *pc_trace*

clean: cleanlogs
	rm -f $(TARGET) $(OBJECTS) $(DEPS)  *.i *.s *.d *.ii

.PHONY: all clean
