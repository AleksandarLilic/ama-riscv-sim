#!/bin/bash

# gets one loop of coremark
# get -4/+4 of the start/stop respectively to profile all loops

#dump=$1
dump="coremark.dump"

p_start=$(rg "iterate\.isra\.0\+0x2c" "$dump" | awk -F, '{print $NF}' | awk '{print $1}')
p_stop=$(rg "iterate\.isra\.0\+0x2c" "$dump" | awk '{print $1}' | awk -F: '{print $1}')

echo "--prof_pc_start $p_start --prof_pc_stop $p_stop"
