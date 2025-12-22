#!/usr/bin/env python3

import argparse

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from utils import get_test_title, smarter_eng_formatter

H = ['rd', 'rdp', 'rs1', 'rs2']
H_ALL_TYPE = ['rd_all', 'rs_all']
H_TOTAL = ['total']
COLUMNS = [H, H_ALL_TYPE, H_TOTAL]

CLRS = ["tab:blue", "tab:green", "tab:orange", "tab:red"]

parser = argparse.ArgumentParser(description="Plot register file usage")
parser.add_argument('prof', help="Input binary profile 'rf_usage.bin'")
parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
parser.add_argument('--save_svg', action='store_true', help="Save charts as SVG")
parser.add_argument('--save_csv', action='store_true', help="Save source data formatted as CSV")
parser.add_argument('--single', default=None, help=f"Index of single column set to plot from {COLUMNS}")

args = parser.parse_args()

rf_names = [
    "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6",
]

dtype = np.dtype([
    (H[0], np.uint64),
    (H[1], np.uint64),
    (H[2], np.uint64),
    (H[3], np.uint64),
])
data = np.fromfile(args.prof, dtype=dtype)
df = pd.DataFrame(data)
df[H_TOTAL[0]] = df.sum(axis=1)
df[H_ALL_TYPE[0]] = df['rd'] + df['rdp']
df[H_ALL_TYPE[1]] = df['rs1'] + df['rs2']

# add first column named 'reg' as 'x' concat with index, e.g. x0, x1, x2, ...
df.insert(0, 'reg', 'x' + df.index.astype(str))

# add a new column with the register names using the index
df['reg_name'] = df.index.map(lambda x: rf_names[x])
df['reg_comb'] = df['reg_name'] + " (" + df['reg'] + ")"
if args.save_csv:
    df.to_csv(args.prof.replace('.bin', '.csv'), index=False)

base_fmt = smarter_eng_formatter()
columns = COLUMNS
if args.single:
    columns = [columns[int(args.single)]]

for col_set in columns:
    fig, ax = plt.subplots(figsize=(12, 10), constrained_layout=True)
    BW = 0.7
    A = 0.55
    y_ax_num = np.arange(df.index.size)
    bars = []
    left = np.zeros(df.index.size)
    for idx, col in enumerate(col_set):
        # replace 0 with np.nan to avoid plotting & labeling zero-height bars
        d = df[col].replace(0, np.nan)
        clr = CLRS[idx%2 + 2] if "rs" in col else CLRS[idx]
        if idx == 0:
            bars.append(ax.barh(y_ax_num, d, BW, label=col, color=clr, alpha=A))
        else:
            bars.append(ax.barh(
                y_ax_num, d, BW, left=left, label=col, color=clr, alpha=A
            ))
        ax.bar_label(bars[-1], fmt=base_fmt, label_type='center', size=8)
        left += df[col].fillna(0).values

    ax.xaxis.set_major_formatter(base_fmt)
    ax.set_yticks(y_ax_num)
    ax.set_yticklabels(df['reg_comb'])
    ax.set_xlabel('Count')
    ax.set_title(f"Register File usage for {get_test_title(args.prof)}\n\n")
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.045), ncol=8)
    ax.margins(y=0.01, x=0)
    ax.grid(axis='x', alpha=0.7)
    #ax.axhspan(20-0.5, 31+0.5, facecolor='lightgray', alpha=0.3, zorder=0)

    name = "_".join(col_set)
    if args.save_png:
        fig.savefig(args.prof.replace(" ", "_").replace(".bin", f"_{name}.png"))
    if args.save_svg:
        fig.savefig(args.prof.replace(" ", "_").replace(".bin", f"_{name}.svg"))
