#!/bin/bash

# script to run self-checking tests on the simulator
# relies on the test writing to 0x340 to indicate success or failure
# test finished: 0x340[[0]] == 0x1
# pass: 0x340 == 0x1
# fail: 0x340 > 0x1, 
# failed test number: 0x340[[31:1]]

binaries_path=$1
sim_executable=$2
START_ADDRESS=0x80000000

if [[ -z "$binaries_path" ]]; then
    echo "Binaries path not provided"
    echo "Usage: $0 <path to binaries> <path to simulator>"
    exit 1
fi
if [[ -z "$sim_executable" ]]; then
    echo "Simulator path not provided"
    echo "Usage: $0 <path to binaries> <path to simulator>"
    exit 1
fi
if [[ ! -f $sim_executable ]]; then
    echo "Simulator not found: $sim_executable"
    exit 1
fi

binaries_found=$(find "$binaries_path" -maxdepth 1 -name "*.bin")
if [[ -z "$binaries_found" ]]; then
    echo "No binaries found in $binaries_path"
    exit 1
fi

run_log="run.log"
failed_runs_log="failed_runs.log"
echo "" > $run_log;
for bin in $binaries_found; do
    echo Running: "$bin"
    echo "$bin" >> $run_log
    $sim_executable "$bin" $START_ADDRESS >> $run_log
done

grep "bin\|0x340" $run_log | grep -v "0x340: 0x1" | grep -B 1 0x340 > $failed_runs_log

total_tests=$(echo "$binaries_found" | wc -l)
echo "Total tests: $total_tests"

if [[ -s $failed_runs_log ]]; then
    echo "Some tests failed"
    echo "Failed tests:"
    cat $failed_runs_log
    exit 1
else
    echo "All tests passed"
    exit 0
fi
