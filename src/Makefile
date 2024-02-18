CXX=g++
CXXFLAGS=-Wall -Wextra -Werror -pedantic -g -std=gnu++17 -O3
TARGET=ama-riscv-sim
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(SOURCES:.cpp=.d)
DEFINES=-DPRINT_EXEC

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ $(DEFINES)

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS)

.PHONY: all clean
