CXX := g++
CXXFLAGS := -Wall -Wextra -Werror -pedantic -std=gnu++17
#CXXFLAGS := -save-temps
CXXFLAGS += -Ofast -s -flto -march=native -mtune=native
#CXXFLAGS += -pg
LDFLAGS :=
TARGET := ama-riscv-sim
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(SOURCES:.cpp=.d)
USER_DEFINES =
DEFINES =
DEFINES += -DLOG_EXEC -DENABLE_DASM
#DEFINES += -DLOG_EXEC_ALL
DEFINES += -DUSE_ABI_NAMES
DEFINES += -DENABLE_PROF
#DEFINES += -DCHECK_LOG
DEFINES += -DUART_ENABLE
#DEFINES += -DUART_INPUT_ENABLE

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES) $(USER_DEFINES)

-include $(DEPS)

INST_LOGS := *inst_profiler.json *inst_profiler.csv *inst_profiler.png
TRACES := *_trace*.csv *_trace*.bin *_trace*.png *_trace*.svg
REG_HIST := *_reg_hist.csv *_reg_hist.bin
HW_STATS := *_hw_stats.json *_perf_est.csv

cleanlogs:
	rm -f *.log $(TRACES) $(REG_HIST) $(INST_LOGS) $(HW_STATS)

clean: cleanlogs
	rm -f $(TARGET)* $(OBJECTS) $(DEPS) *.i *.s *.d *.ii *.gc*

.PHONY: all clean
