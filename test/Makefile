CXX := g++
CXXFLAGS := -lgtest -lgtest_main -pthread
TARGET := ama-riscv-sim_test
SOURCES := test.cpp
PYVER := python3

all: build_sim prepare run

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) -o $@ $^ $(CXXFLAGS)

prepare:
	$(PYVER) prepare_riscv_tests.py --testlist testlist.json --isa_tests

build_sim:
	make -C ../src --no-print-directory clean
	make -C ../src --no-print-directory -j DEFINES=-DUART_ENABLE

clean:
	rm -rf $(TARGET) *.o *.d *.i *.s *.ii *_dump.log

.PHONY: all clean prepare
