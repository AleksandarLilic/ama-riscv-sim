#!/usr/bin/env python3
import os
import sys

import numpy as np
from run_analysis import BASE_ADDR, CACHE_LINE_BYTES, hex2int
from utils import INDENT

if len(sys.argv) < 2 or len(sys.argv) > 3:
    print(f"Usage: {sys.argv[0]} <dasm_file> [boundary_bytes]")
    sys.exit(1)

dasm = sys.argv[1]
if not os.path.isfile(dasm):
    print(f"File {dasm} does not exist.")
    sys.exit(1)

BOUNDARY = int(sys.argv[2]) if len(sys.argv) == 3 else CACHE_LINE_BYTES
if np.log2(BOUNDARY) % 1 != 0:
    print(f"Boundary '{BOUNDARY}' is not a power of 2.")
    sys.exit(1)

SECTION = 'text'
inst_map = []
pc_arr = []
with open(dasm, 'r') as infile:
    found_text_section = False
    for line in infile:
        found_section = line.startswith('Disassembly of section .')
        if found_section:
            found_text_section = (SECTION in line)

        if found_text_section and line.strip():
            parts = line.split(':')
            if (len(parts) == 2) and parts[1].startswith('\t'):
                pc_arr.append(hex2int(parts[0].strip()) - BASE_ADDR)
                inst_mn = line.split('\t')
                inst_mn = [x.strip() for x in inst_mn]
                inst_map.append(
                    [hex2int(inst_mn[0].replace(':', '')), # pc
                    inst_mn[1], # instruction
                    inst_mn[2], # instruction mnemonic
                    ' '.join(inst_mn[2:]) # full asm
                ])

print(f"Breakdown for boundary {BOUNDARY} bytes and dasm '{dasm}':")
pca = np.array(pc_arr)
pca = pca % BOUNDARY

pattern = np.array([BOUNDARY-4, 0])
indices = np.where((pca[:-1] == pattern[0]) & (pca[1:] == pattern[1]))[0]
print(f"{INDENT}4B inst aligned:    {len(indices)}")
#print(indices)

pattern = np.array([BOUNDARY-2, 2])
indices = np.where((pca[:-1] == pattern[0]) & (pca[1:] == pattern[1]))[0]
print(f"{INDENT}4B inst misaligned: {len(indices)}")
#print(indices)

pattern = np.array([BOUNDARY-2, 0])
indices = np.where((pca[:-1] == pattern[0]) & (pca[1:] == pattern[1]))[0]
print(f"{INDENT}2B inst:            {len(indices)}")
#print(indices)

inst_rv32c = [mnm for pc, inst, mnm, full in inst_map if mnm.startswith('c.')]
inst_rv32i = [mnm for pc, inst, mnm, full in inst_map
              if not ((mnm.startswith('c.') or inst == "0000"))]
inst_padding = [mnm for pc, inst, mnm, full in inst_map if inst == "0000"]

num_inst_rv32c = len(inst_rv32c)
size_inst_rv32c = num_inst_rv32c * 2 # 2 bytes each
num_inst_rv32i = len(inst_rv32i)
size_inst_rv32i = num_inst_rv32i * 4 # 4 bytes each

num_inst_total = num_inst_rv32c + num_inst_rv32i
text_size = size_inst_rv32c + size_inst_rv32i
text_size_4b = num_inst_total * 4 # if all were 4 byte instructions

print(f"Size breakdown:")
rv32c_perc = num_inst_rv32c / num_inst_total * 100
print(f"{INDENT}RV32C instructions % in code: {rv32c_perc:.2f}%")
text_reduction = (text_size_4b - text_size) / text_size_4b * 100
print(f"{INDENT}Reduction (half the above):   {text_reduction:.2f}%")
