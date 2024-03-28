#!/bin/bash

START_ADDR="0x80000000"

RV_BIN=$1
# check if the binary exists
if [ ! -f "$RV_BIN" ]; then
    echo "Invalid file: $RV_BIN. Please provide a valid RISC-V binary as the first argument."
    exit 1
fi

# take number of seconds from command line, default to 10 if not provided
n=${2:-10}
if ! [[ "$n" =~ ^[0-9]+$ ]]; then
    echo "Invalid number of iterations: $n. Please provide a positive integer as the second argument."
    exit 1
fi

# take param (user or real) from command line, default to real if not provided
param=${3:-real}
if [[ "$param" != "user" && "$param" != "real" ]]; then
    echo "Invalid param: $param. Please provide either 'user' or 'real' as the third argument."
    exit 1
fi

REPO_ROOT=$(git rev-parse --show-toplevel)
EXEC_BIN="$REPO_ROOT/src/ama-riscv-sim"
RUN_SIM="$EXEC_BIN $RV_BIN $START_ADDR"

# get instruction count for the initial iteration
INST_CNT=$($RUN_SIM | grep "Inst Counter:" | awk '{print $3}')
echo "Inst count: $INST_CNT"

# now time it
param_sum=0
param_sum_mips=0
for i in $(seq 1 "$n"); do
    tot_time=$( { time $RUN_SIM 2>&1 >/dev/null; } 2>&1 | grep "real\|user\|sys")
    param_time=$(echo "$tot_time" | grep "$param" | awk '{print $2}')
    minutes=$(echo "$param_time" | cut -d'm' -f1)
    seconds=$(echo "$param_time" | cut -d'm' -f2 | sed 's/s//')
    
    param_seconds=$(echo "$minutes * 60 + $seconds" | bc)
    param_mips=$(echo "scale=0; $INST_CNT / $param_seconds / 1000000" | bc)
    echo "loop $i: $param: $param_seconds, MIPS: $param_mips"
    
    param_sum=$(echo "$param_sum + $param_seconds" | bc)
    param_sum_mips=$(echo "$param_sum_mips + $param_mips" | bc)
done

param_avg=$(echo "scale=3; $param_sum / $n" | bc)
param_avg_mips=$(echo "scale=0; $param_sum_mips / $n" | bc)
echo "avg: $param: $param_avg, MIPS: $param_avg_mips"
