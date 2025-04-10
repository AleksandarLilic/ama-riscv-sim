CXX := g++
CXXFLAGS := -g -std=gnu++17
GTEST_LIBS := -lgtest -lgtest_main -pthread
LDFLAGS := -lgcov
SIM_FLAGS := DEFINES="-DTEST_BUILD -DRV32C"
SIM_FLAGS += USER_DEFINES=--coverage LDFLAGS=-lgcov
TEST := ama-riscv-sim_test
BDIR := build_gtest
SOURCES := test.cpp
PYVER := python3
COVDIR := coverage
COVLOG := coverage.log
VALGRIND := valgrind --leak-check=full

all: coverage

run_gtest: build_sim prepare_tests $(TEST)
	./$(TEST)

coverage: run_gtest
	@mkdir -p $(COVDIR)
	lcov --capture --directory ../src/$(BDIR) --output-file $(COVDIR)/coverage.info > $(COVLOG) 2>&1
	genhtml $(COVDIR)/coverage.info --output-directory $(COVDIR) >> $(COVLOG) 2>&1
	@tail -n 3 $(COVLOG)

run_valgrind: $(TEST)
	$(VALGRIND) ./$(TEST)

$(TEST): $(SOURCES)
	$(CXX) -o $@ $^ $(GTEST_LIBS) $(CXXFLAGS)
	@echo "Gtest build done."

NO_PREP ?= 0
prepare_tests:
	@if [ "$(NO_PREP)" = "0" ]; then \
		$(PYVER) prepare_riscv_tests.py --testlist testlist.json --isa_tests; \
		echo "RISC-V tests build done."; \
	fi

clean_tests:
	$(PYVER) prepare_riscv_tests.py --testlist testlist.json --isa_tests --clean_only

build_sim:
	make -C ../src --no-print-directory BDIR=$(BDIR) NO_LINK=1 -j $(SIM_FLAGS)
	@echo "Simulator build done."

cleanlogs:
	rm -rf *.log out_*

cleanbins:
	rm -f unsupported*.bin oversized.bin

clean: cleanlogs cleanbins
	rm -rf $(TEST) $(COVDIR) *.gc* gtest_testlist.txt

cleanall: clean
	make -C ../src --no-print-directory clean

.PHONY: all run_gtest coverage run_valgrind prepare_tests clean_tests build_sim cleanlogs cleanbins clean cleanall
