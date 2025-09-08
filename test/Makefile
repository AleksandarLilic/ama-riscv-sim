CXX := g++
CXXFLAGS := -g -std=gnu++17
GTEST_LIBS := -lgtest -lgtest_main -pthread
LDFLAGS := -lgcov
SIM_FLAGS := DEFINES="-DTEST_BUILD -DRV32C"
SIM_FLAGS += USER_DEFINES=--coverage LDFLAGS=-lgcov

TIMESTAMP := $(shell date +"%Y-%m-%d_%H-%M-%S")
TEST_DIR = testrun_$(TIMESTAMP)
$(shell mkdir -p $(TEST_DIR))
TEST_BIN := ama-riscv-sim_test
TEST := $(TEST_DIR)/$(TEST_BIN)
BDIR := build_gtest
SOURCES := test.cpp
PYVER := python3
COVDIR := $(TEST_DIR)/coverage
COVLOG := $(TEST_DIR)/coverage.log
VALGRIND := valgrind --leak-check=full

all: coverage

build_test: $(TEST)

run_gtest: build_sim $(TEST)
	cd $(TEST_DIR) && ./$(TEST_BIN) && cd -

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

prepare_tests:
	@./prepare_riscv_tests.py --testlist testlist.json --isa_tests
	echo "RISC-V tests build done."

clean_tests:
	@./prepare_riscv_tests.py --testlist testlist.json --isa_tests --clean_only

build_sim:
	make -C ../src --no-print-directory BDIR=$(BDIR) NO_SYMLINK=1 -j $(SIM_FLAGS)
	@echo "Simulator build done."

cleanlogs:
	rm -rf *.log out_*

cleanbins:
	rm -f unsupported*.bin oversized.bin

cleantestruns:
	rm -rf testrun_*

clean: cleanlogs cleanbins
	rm -rf $(TEST) $(COVDIR) *.gc* gtest_testlist.txt

cleanall: clean
	make -C ../src --no-print-directory clean

.PHONY: all run_gtest coverage run_valgrind prepare_tests clean_tests build_sim cleanlogs cleanbins clean cleanall
