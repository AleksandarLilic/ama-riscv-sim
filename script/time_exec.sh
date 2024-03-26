#!/bin/bash

# take number of seconds from command line, default to 10 if not provided
n=${1:-10}
if ! [[ "$n" =~ ^[0-9]+$ ]]; then
    echo "Invalid number of iterations: $n. Please provide a positive integer as the first argument."
    exit 1
fi

# take param (user or real) from command line, default to real if not provided
param=${2:-real}
if [[ "$param" != "user" && "$param" != "real" ]]; then
    echo "Invalid param: $param. Please provide either 'user' or 'real' as the second argument."
    exit 1
fi

# FIXME: relies on being run from src directory
RV_BIN="../sw/baremetal/asm_test/asm_test.bin"
EXEC_BIN="./ama-riscv-sim"
START_ADDR="0x80000000"
param_sum=0
for i in $(seq 1 "$n"); do
    tot_time=$( { time $EXEC_BIN $RV_BIN $START_ADDR 2>&1 >/dev/null; } 2>&1 | grep "real\|user\|sys")
    param_time=$(echo "$tot_time" | grep "$param" | awk '{print $2}')
    minutes=$(echo "$param_time" | cut -d'm' -f1)
    seconds=$(echo "$param_time" | cut -d'm' -f2 | sed 's/s//')
    param_seconds=$(echo "$minutes * 60 + $seconds" | bc)
    echo "iteration $i: $param: $param_seconds"
    param_sum=$(echo "$param_sum + $param_seconds" | bc)
done

param_avg=$(echo "scale=3; $param_sum / $n" | bc)
echo "avg: $param: $param_avg"
