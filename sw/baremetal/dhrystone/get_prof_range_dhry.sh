#!/bin/bash

# gets one loop of dhrystone
# get -4/+4 of the start/stop respectively to profile all loops

#dump=$1
dump="dhrystone.dump"

p_start=$(rg "jal.*Proc_5" "$dump" | awk '{print $1}' | awk -F: '{print $1}')
p_stop=$(rg "bne.*${p_start}" "$dump" | awk '{print $1}' | awk -F: '{print $1}')

echo "--prof_pc_start $p_start --prof_pc_stop $p_stop"
