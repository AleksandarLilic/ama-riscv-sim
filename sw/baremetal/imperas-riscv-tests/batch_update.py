#!/usr/bin/env python3

# Script for adapting imperas tests to ama-riscv-sim env

import glob

# string to search for in each .S file
TARGET_STRING = '#include "model_test.h"'

# replacement block
REPLACEMENT_BLOCK = '''
#include "../common/asm_test.S"
#include "../common/csr.h"

#include "model_test.h"
#include "arch_test.h"
RVTEST_ISA("RV32I")

.section .text
.global main
main:
'''

# number of lines to delete after the target line (default: 8)
DELETE_COUNT = 8

def process_file(path: str) -> None:
    """
    Open the file at `path`, remove the line containing TARGET_STRING and
    the next DELETE_COUNT lines, then insert REPLACEMENT_BLOCK at that spot.
    """
    with open(path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        if TARGET_STRING in lines[i]:
            # insert replacement block
            block = REPLACEMENT_BLOCK
            new_lines.append(block)
            # skip the matched line and the next DELETE_COUNT lines
            i += 1 + DELETE_COUNT
        else:
            new_lines.append(lines[i])
            i += 1

    # Overwrite the original file
    with open(path, 'w', encoding='utf-8') as f:
        f.writelines(new_lines)

def main():
    files = glob.glob('*.S')
    files.sort()
    for filepath in files:
        print(f"Processing {filepath} ...")
        process_file(filepath)
    print(f"Finished {len(files)} files")

if __name__ == '__main__':
    main()
