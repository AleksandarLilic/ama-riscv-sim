CXX := g++
CXXFLAGS := -g -std=gnu++17
GTEST_LIBS := -lgtest -lgtest_main -pthread
LDFLAGS := -lgcov
SIM_FLAGS := DEFINES="-DUART_ENABLE -DTEST_BUILD" USER_DEFINES=--coverage LDFLAGS=-lgcov
TEST := ama-riscv-sim_test
SOURCES := test.cpp
PYVER := python3
COVDIR := coverage
COVLOG := coverage.log
VALGRIND := valgrind --leak-check=full
NO_PREP ?= 0

all: build_sim prepare_tests coverage

run_gtest: $(TEST)
	./$(TEST)

coverage: run_gtest
	@mkdir -p $(COVDIR)
	lcov --capture --directory ../src --output-file $(COVDIR)/coverage.info > $(COVLOG) 2>&1
	genhtml $(COVDIR)/coverage.info --output-directory $(COVDIR) >> $(COVLOG) 2>&1
	@tail -n 3 $(COVLOG)

run_valgrind: $(TEST)
	$(VALGRIND) ./$(TEST)

$(TEST): $(SOURCES)
	$(CXX) -o $@ $^ $(GTEST_LIBS) $(CXXFLAGS)

prepare_tests:
	@if [ "$(NO_PREP)" = "0" ]; then \
		$(PYVER) prepare_riscv_tests.py --testlist testlist.json --isa_tests; \
	fi

clean_tests:
	$(PYVER) prepare_riscv_tests.py --testlist testlist.json --isa_tests --clean_only

build_sim:
	make -C ../src --no-print-directory clean
	make -C ../src --no-print-directory -j $(SIM_FLAGS)

cleanlogs:
	rm -f *.log *_profiler.* *trace* *_perf_est.csv

cleanbins:
	rm -f unsupported*.bin oversized.bin

LOGS = *_dump.log *_hw_stats.json
clean: cleanlogs cleanbins
	rm -rf $(TEST) *.o *.d *.i *.s *.ii  $(LOGS) $(COVDIR) *.gc* gtest_testlist.txt

cleanall: clean
	make -C ../src --no-print-directory clean

.PHONY: all clean prepare
