#!/usr/bin/env python3

import os
import sys
import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description="Plot register file usage")
parser.add_argument('log', help="Input binary log from profiler")
parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
parser.add_argument('--save_svg', action='store_true', help="Save charts as SVG")
parser.add_argument('--save_csv', action='store_true', help="Save source data formatted as CSV")

args = parser.parse_args()

rf_names = [
    "zero","ra","sp","gp","tp","t0","t1","t2","s0","s1","a0","a1","a2","a3","a4","a5",
    "a6","a7","s2","s3","s4","s5","s6","s7","s8","s9","s10","s11","t3","t4","t5","t6"
]

h = ['rd', 'rs1', 'rs2']
dtype = np.dtype([
    (h[0], np.uint32),
    (h[1], np.uint32),
    (h[2], np.uint32),
])
data = np.fromfile(args.log, dtype=dtype)
df = pd.DataFrame(data, columns=h)
df['total'] = df.sum(axis=1)
df['rs'] = df['rs1'] + df['rs2']
# add first column named 'reg' as 'x' concat with index, e.g. x0, x1, x2, ...
df.insert(0, 'reg', 'x' + df.index.astype(str))
# add a new column with the register names using the index
df['reg_name'] = df.index.map(lambda x: rf_names[x])
df['reg_comb'] = df['reg_name'] + " (" + df['reg'] + ")"
if args.save_csv:
    df.to_csv(args.log.replace('.bin', '.csv'), index=False)

columns = [['rd', 'rs1', 'rs2'], ['rd', 'rs'], ['total']]
for col_set in columns:
    fig, ax = plt.subplots(figsize=(12, 10), constrained_layout=True)
    bar_width = 0.7
    alpha = 0.55
    y_axis_numeric = np.arange(df.index.size)
    bars = []
    for idx, col in enumerate(col_set):
        if idx == 0:
            bars.append(ax.barh(y_axis_numeric, df[col], bar_width,
                                label=col, alpha=alpha))
        else:
            bars.append(ax.barh(y_axis_numeric, df[col], bar_width,
                                left=df[df.columns[1:idx+1]].sum(axis=1),
                                label=col, alpha=alpha))
        ax.bar_label(bars[-1], fmt='%.0f', label_type='center', size=8)

    ax.set_yticks(y_axis_numeric)
    ax.set_yticklabels(df['reg_comb'])
    ax.set_xlabel('Count')
    ax.set_title("Register File usage\n\n")
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.045), ncol=8)
    ax.margins(y=0.01, x=0)
    ax.grid(axis='x', alpha=0.7)
    ax.axhspan(20-0.5, 31+0.5, facecolor='lightgray', alpha=0.6, zorder=0)

    name = "_".join(col_set)
    if args.save_png:
        fig.savefig(args.log.replace(" ", "_").replace(".bin", f"_{name}.png"))
    if args.save_svg:
        fig.savefig(args.log.replace(" ", "_").replace(".bin", f"_{name}.svg"))
