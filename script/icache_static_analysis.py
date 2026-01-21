#!/usr/bin/env python3
import os
import sys

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from run_analysis import BASE_ADDR, CACHE_LINE_BYTES, hex2int, icfg
from utils import INDENT

if len(sys.argv) < 2 or len(sys.argv) > 3:
    print(f"Usage: {sys.argv[0]} <dasm_file> [icache_size_size_bytes]")
    sys.exit(1)

dasm = sys.argv[1]
if not os.path.isfile(dasm):
    print(f"File {dasm} does not exist.")
    sys.exit(1)

IC_SIZE = int(sys.argv[2]) if len(sys.argv) == 3 else CACHE_LINE_BYTES
if np.log2(IC_SIZE) % 1 != 0:
    print(f"Icache line size '{IC_SIZE}' is not a power of 2.")
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

print(f"Breakdown for Icache line {IC_SIZE} bytes and dasm '{dasm}':")
pca = np.array(pc_arr)
pca = pca % IC_SIZE

df_im = pd.DataFrame(inst_map, columns=['pc', 'inst', 'mnm', 'full_asm'])
df_im['CL'] = (df_im.pc - BASE_ADDR) // IC_SIZE
# create a boolean column indicating whether the mnm is in given array
df_im['is_branch'] = df_im['mnm'].isin(icfg.INST_T[icfg.k_branch])
df_im['is_jump_d'] = df_im['mnm'].isin(icfg.INST_T_JUMP[icfg.k_jump_direct])
df_im['is_jump_i'] = df_im['mnm'].isin(icfg.INST_T_JUMP[icfg.k_jump_indirect])

branch_dist = df_im.groupby('CL').agg({'is_branch': 'sum'}).reset_index()
branch_dist = branch_dist.rename(columns={'is_branch': 'num_branches'})

jump_d_dist = df_im.groupby('CL').agg({'is_jump_d': 'sum'}).reset_index()
jump_d_dist = jump_d_dist.rename(columns={'is_jump_d': 'num_jumps_d'})

jump_i_dist = df_im.groupby('CL').agg({'is_jump_i': 'sum'}).reset_index()
jump_i_dist = jump_i_dist.rename(columns={'is_jump_i': 'num_jumps_i'})

br_dist = branch_dist.merge(jump_d_dist, on='CL', how='outer')
br_dist = br_dist.merge(jump_i_dist, on='CL', how='outer')
br_dist['all_flow'] = (
    br_dist['num_branches'] + br_dist['num_jumps_d'] + br_dist['num_jumps_i'])

fig, ax = plt.subplots(figsize=(10, 6))
ax.bar(br_dist['CL'], br_dist['num_branches'],
       label='Branch', alpha=0.7)
ax.bar(br_dist['CL'], br_dist['num_jumps_d'],
       bottom=br_dist['num_branches'],
       label='Jump Direct', alpha=0.7)
ax.bar(br_dist['CL'], br_dist['num_jumps_i'],
       bottom=br_dist['num_branches'] + br_dist['num_jumps_d'],
       label='Jump Indirect', alpha=0.7)

ax.set_xlabel('Icache Line Number')
ax.set_ylabel('Number of Branches/Jumps')
ax.set_title('Distribution of Branch and Jump Instructions per Icache Line')
ax.legend()
plt.tight_layout()
ax.margins(x=0)
ax.grid(axis='y')
avg_branches = br_dist['num_branches'].mean()
avg_jumps_d = br_dist['num_jumps_d'].mean()
avg_jumps_in = br_dist['num_jumps_i'].mean()
avg_all_flow = br_dist['all_flow'].mean()
print(f"Average flow change instructions per Icache line:")
print(f"{INDENT}Branch:   {avg_branches:.2f}")
print(f"{INDENT}Jump D:   {avg_jumps_d:.2f}")
print(f"{INDENT}Jump I:   {avg_jumps_in:.2f}")
print(f"{INDENT}All flow: {avg_all_flow:.2f}")

print(f"Compressed instructions distribution:")
pattern = np.array([IC_SIZE-4, 0])
indices = np.where((pca[:-1] == pattern[0]) & (pca[1:] == pattern[1]))[0]
print(f"{INDENT}4B aligned:    {len(indices)}")
#print(indices)

pattern = np.array([IC_SIZE-2, 2])
indices = np.where((pca[:-1] == pattern[0]) & (pca[1:] == pattern[1]))[0]
print(f"{INDENT}4B misaligned: {len(indices)}")
#print(indices)

pattern = np.array([IC_SIZE-2, 0])
indices = np.where((pca[:-1] == pattern[0]) & (pca[1:] == pattern[1]))[0]
print(f"{INDENT}2B:            {len(indices)}")
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
