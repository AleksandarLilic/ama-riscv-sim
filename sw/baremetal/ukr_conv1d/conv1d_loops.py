#!/usr/bin/env python3

"""Debug helper script for figuring out the loop limits for SIMD convolution
"""

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-e", "--el_width", type=int, default=16)
parser.add_argument("-i", "--in_len", type=int, default=16)
parser.add_argument("-f", "--f_len", type=int, default=5)

args = parser.parse_args()

EL_WIDTH = args.el_width
IN_LEN = args.in_len
F_LEN = args.f_len

if EL_WIDTH not in (16, 8, 4, 2):
    raise ValueError(f"Unsupported element width '{EL_WIDTH}'")

if IN_LEN < 1:
    raise ValueError(f"Input length must be >=1, set at '{IN_LEN}'")

if F_LEN < 1:
    raise ValueError(f"Filter length must be >=1, set at '{F_LEN}'")

def round_up(a, len):
    return ((a + (len- 1)) // len) * len

def round_down(a, len):
    return (a // len) * len

VL = (32 // EL_WIDTH)
FP_LEN = round_up((F_LEN + VL - 1), VL)
OUT_LEN = (IN_LEN - F_LEN + 1)

# how far the fast loop may go, one limit per array, write and read guards
rem = (OUT_LEN % VL) # last partial group can't do entire VL
lim_wr = (OUT_LEN - rem) # last whole group out[] can hold
lim_rd = (round_down(IN_LEN - FP_LEN, VL) + VL) if (IN_LEN >= FP_LEN) else 0 # last whole group in[] can feed
n_fast = min(lim_rd, lim_wr)
tail = OUT_LEN - n_fast

print(f"EL_WIDTH = {EL_WIDTH}", end=', ')
print(f"IN_LEN = {IN_LEN}", end=', ')
print(f"F_LEN = {F_LEN}", end=', ')
print(f"OUT_LEN = {OUT_LEN}", end=', ')
print(f"VL = {VL}", end=', ')
print(f"FP_LEN = {FP_LEN}")
print(f"rem = {rem}", end=', ')
print(f"lim_wr = {lim_wr}", end=', ')
print(f"lim_rd = {lim_rd}", end=', ')
print(f"n_fast = {n_fast} ({n_fast//VL} loops)", end=', ')
print(f"k = {FP_LEN} (fp_len)")

print("tail", tail)
for n in range(n_fast, OUT_LEN):
    p = (n % VL) # phase
    a_idx = (n - p) # input, aligned
    k = (IN_LEN - (n - p))
    print(f"    n={n}, p={p}, ai={a_idx}, k={k}")
