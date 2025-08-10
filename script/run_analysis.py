#!/usr/bin/env python3

import argparse
import json
import math
import os
import textwrap
from typing import Any, Dict, List, Tuple

import matplotlib.patches as patches
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.ticker import (FuncFormatter, LogFormatter,
                               LogFormatterSciNotation, MaxNLocator,
                               MultipleLocator)
from utils import is_notebook

#SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
ARITH = "ARITH"
BITMANIP = "BITMANIP"
UNPAK = "UNPAK"
MEM = "MEM"
MEM_HINTS = "MEM_HINTS"
BRANCH = "BRANCH"
JUMP = "JUMP"
CSR = "CSR"
ENV = "ENV"
NOP = "NOP"
FENCE = "FENCE"
MEM_S = "MEM_S"
MEM_L = "MEM_L"

CACHE_LINE_BYTES = 64
BASE_ADDR = 0x40000
MEM_SIZE = 65536

inst_t_mem = {
    MEM_S: ["sb", "sh", "sw", "c.swsp", "c.sw"],
    MEM_L: ["lb", "lh", "lw", "lbu", "lhu", "c.lwsp", "c.lw", "c.li"],
}

inst_t = {
    ARITH: [
        "add", "sub", "sll", "srl", "sra", "slt", "sltu", "xor", "or", "and",
        "addi", "slli", "srli", "srai", "slti", "sltiu", "xori", "ori", "andi",
        "lui", "auipc",
        "mul", "mulh", "mulhsu", "mulhu", "div", "divu", "rem", "remu",
        "c.add", "c.addi", "c.addi16sp", "c.addi4spn", "c.sub",
        "c.andi", "c.srli", "c.slli", "c.srai", "c.and", "c.xor", "c.or",
        "c.mv",
        "c.lui",
        "dot4", "dot8", "dot16",
    ],
    BITMANIP: [ "max", "maxu", "min", "minu"],
    UNPAK: [
        "unpk16", "unpk16u", "unpk8", "unpk8u",
        "unpk4", "unpk4u", "unpk2", "unpk2u",
    ],
    MEM: inst_t_mem[MEM_S] + inst_t_mem[MEM_L],
    MEM_HINTS: ["scp.ld", "scp.rel"],
    BRANCH: ["beq", "bne", "blt", "bge", "bltu", "bgeu", "c.beqz", "c.bnez"],
    JUMP: ["jalr", "jal","c.j" ,"c.jal" ,"c.jr" ,"c.jalr"],
    CSR: ["csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"],
    ENV: ["ecall", "ebreak", "c.ebreak"],
    NOP: ["nop", "c.nop"],
    FENCE: ["fence.i"],
}

all_inst = []
for k, v in inst_t.items():
    all_inst.extend(v)

all_inst_types = list(inst_t.keys()) + list(inst_t_mem.keys())

# create a reverse map for instruction types
inst_t_map = {}
for k, v in inst_t.items():
    for inst in v:
        inst_t_map[inst] = k

inst_t_mem_map = {}
for k, v in inst_t_mem.items():
    for inst in v:
        inst_t_mem_map[inst] = k

# TODO: add function calls/graphs highlight when supported by the profiler
colors = {
    "blue_base": "#0077b6",
    "blue_light1": "#00b4d8",
    "blue_light2": "#90e0ef",
}

hl_colors = [
    "#3ECCBB", # turquoise
    "#EED595", # peach yellow
    "#f4a261", # orange
    "#e76f51", # red
    "#538D85", # teal
]

# memory instructions breakdown store vs load
inst_mem_bd = {
    MEM_L : ["Load", colors["blue_light1"]],
    MEM_S : ["Store", colors["blue_light2"]],
}

def get_base_int_addr(addr) -> int:
    return int(addr,16) # - int(BASE_ADDR)

def get_count(parts, df) -> Tuple[int, int]:
    pc = get_base_int_addr(parts[0].strip())
    count_series  = df.loc[df["pc"] == pc, "count"]
    count = count_series.squeeze() if not count_series.empty else 0
    return count, pc

def num_to_hex(val, pos) -> str:
    return f"0x{int(val):04X}"

def to_k(val, pos) -> str:
    if val == 0:
        return "0"
    if val/1000 == val//1000:
        return f"{val//1000:.0f}k"
    return f"{val/1000:.3f}k"

def inst_exists(inst) -> str:
    if inst not in all_inst:
        raise ValueError(f"Invalid instruction '{inst}'. " + \
                         f"Available instructions are: {', '.join(all_inst)}")
    return inst

def inst_type_exists(inst_type) -> str:
    if inst_type not in all_inst_types:
        raise ValueError(f"Invalid instruction type '{inst_type}'. " + \
                         f"Available types are: {', '.join(all_inst_types)}")
    return inst_type

def find_loc_range(ax) -> int:
    ymin, ymax = ax.get_ylim()
    yrange = ymax - ymin
    inc = 1
    if yrange > 1000:
        inc = 4
    if yrange > 10_000:
        inc = 16
    if yrange > 100_000:
        inc = 32
    return inc

def wrap_label(arr:List[str], max_len:int) -> str:
    label = ', '.join(arr)
    wrapped = '\n'.join(textwrap.wrap(label, max_len))
    return wrapped

def add_legend_for_hl_groups(ax, chart_type:str) -> None:
    title = "Highlighted\nInstructions"
    if chart_type == "log":
        ax.legend(loc='lower right', title=title, framealpha=0.5)
    elif chart_type == "trace":
        # put legend in the top right corner, outside the plot
        ax.legend(loc='upper left', title=title, framealpha=0.5,
                  bbox_to_anchor=(1.01, 1.03), borderaxespad=0.)
    else:
        raise ValueError(f"Invalid chart type '{chart_type}'")

def ctype_check(ctype:str) -> Tuple[List[str], str]:
    if ctype == 'pc':
        cols = ['pc', 'isz', 'count']
        ylabel = "Program Counter"
    elif ctype == 'dmem':
        cols = ['dmem', 'dsz', 'count']
        ylabel = "Data Memory"
    else:
        raise ValueError(f"Invalid chart type '{ctype}'. " + \
                         f"Available types are: 'pc', 'dmem'")

    return cols, ylabel

def draw_inst_log(df, hl_groups, title, args) -> plt.Figure:
    inst_profiled = df['count'].sum()
    df['i_type'] = df['name'].map(inst_t_map)
    df['i_mem_type'] = df['name'].map(inst_t_mem_map)

    # filter out instructions if needed
    if args.exclude:
        df = df[~df['name'].isin(args.exclude)]

    if args.exclude_type:
        df = df[~df['i_type'].isin(args.exclude_type)]

    if args.top:
        df = df.tail(args.top)
    if not args.allow_zero:
        df = df[df['count'] != 0]

    # separate the instructions by type
    df_g = df[['i_type', 'count']].groupby('i_type').sum()
    df_g = df_g.sort_values(by='count', ascending=True)

    # separate the memory instructions by type
    df_mem_g = df[['i_mem_type', 'count']].groupby('i_mem_type').sum()
    df_mem_g = df_mem_g.sort_values(by='count', ascending=False)

    # add a bar chart
    ROWS = 2
    COLS = 1
    box = []
    fig, ax = plt.subplots(ROWS, COLS,
                           figsize=(COLS*10, ROWS*(len(df)+len(df_g))/6),
                           height_ratios=(len(df)*.95, len(df_g)),
                           constrained_layout=True)
    suptitle_str = f"Execution profile for {title}"
    suptitle_str += f"\nSamples profiled: {inst_profiled}"
    if args.exclude or args.exclude_type:
        suptitle_str += f" ({df['count'].sum()} shown, "
        suptitle_str += f"{inst_profiled - df['count'].sum()} excluded)"

    fig.suptitle(suptitle_str, size=12)
    box.append(ax[0].barh(df['name'], df['count'], color=colors["blue_base"]))
    box.append(ax[1].barh(df_g.index, df_g['count'], color=colors["blue_base"]))
    y_ax0_offset = min(0.025, len(df)/2_000)
    ax[0].margins(y=0.03-y_ax0_offset)
    ax[1].margins(y=0.03)

    for i in range(ROWS):
        ax[i].bar_label(box[i], padding=3)
        ax[i].set_xlabel('Count')
        ax[i].grid(axis='x')
        ax[i].margins(x=0.06)

    # highlight specific instructions, if any
    hc = 0
    for hl_g in hl_groups:
        for i, r in enumerate(box[0]):
            if df.iloc[i]['name'] in hl_g:
                r.set_color(hl_colors[hc])
        ax[0].barh(0, 0, color=hl_colors[hc], label=', '.join(hl_g))
        hc += 1

    if len(hl_groups) > 0:
        add_legend_for_hl_groups(ax[0], "log")

    # add memory instructions breakdown, if any
    df_mem_g = df_mem_g[df_mem_g['count'] != 0] # never label if count is zero
    if len(df_mem_g) > 0:
        mem_type_index = df_g.index.get_loc("MEM")
        left_start = 0
        for i, row in df_mem_g.iterrows():
            rect_m = ax[1].barh(mem_type_index, row['count'], left=left_start,
                                label=inst_mem_bd[row.name][0],
                                color=inst_mem_bd[row.name][1])
            ax[1].bar_label(rect_m, padding=0, label_type='center', size=7)
            left_start += row['count']

        ax[1].legend(loc='lower right')

    return fig

def json_prof_to_df(log, allow_internal=False) -> pd.DataFrame:
    ar = []
    with open(log, 'r') as file:
        data = json.load(file)
        for key in data:
            if key.startswith('_'):
                if allow_internal:
                    ar.append([key, data[key]])
                continue
            ar.append([key, data[key]['count']])

    df = pd.DataFrame(ar, columns=['name', 'count'])
    df['count'] = df['count'].astype(int)
    df = df.sort_values(by='count', ascending=True)
    return df

def run_inst_log(log, hl_groups, title, args) -> \
Tuple[pd.DataFrame, plt.Figure]:

    df = json_prof_to_df(log)
    fig = draw_inst_log(df, hl_groups, title, args)

    return df, fig

def backannotate_dasm(args, df, section) -> \
Tuple[Dict[str, Dict[str, int]], pd.DataFrame]:

    symbols = {}
    pc_inst_map_arr = []
    logs_path = os.path.dirname(args.trace)
    os.system(f"cp {args.dasm} {logs_path}") # copy og for reference
    dasm_name = os.path.basename(args.dasm)
    dasm_ext = os.path.splitext(dasm_name)[1]
    new_dasm_ext = ".prof" + dasm_ext
    if section == "data":
        # avoid writing to the inst annotated file, no annotations for data
        new_dasm_ext = ".dummy" + dasm_ext

    outfile_name = dasm_name.replace(dasm_ext, new_dasm_ext)
    outfile_name = os.path.join(logs_path, outfile_name)
    with open(args.dasm, 'r') as infile, open(outfile_name, 'w') as outfile:
        current_sym = None
        append = False
        PADDING = len(str(df['count'].max())) + 1
        prev_addr = None
        for line in infile:
            if line.startswith('Disassembly of section .') and section in line:
                append = True
                outfile.write(line)
                continue
            elif line.startswith('Disassembly of section .'):
                append = False

            if append and line.strip():
                parts = line.split(':')

                if len(parts) == 2 and parts[1].startswith('\n'):
                    # detected symbol
                    parts = parts[0].split(" ")
                    addr_start = get_base_int_addr(parts[0].strip())
                    symbol_name = parts[1][1:-1] # remove <> from symbol name
                    if current_sym: # close previous section
                        symbols[current_sym]['addr_end'] = prev_addr
                    current_sym = symbol_name
                    symbols[current_sym] = {
                        'addr_start': addr_start,
                        "exec_count": 0
                    }
                    prev_addr = addr_start

                if len(parts) == 2 and parts[1].startswith('\t'):
                    if section == "text":
                        # detected instruction
                        count, prev_addr = get_count(parts, df)
                        outfile.write("{:{}} {}".format(count, PADDING, line))
                        symbols[current_sym]['exec_count'] += count

                        inst_mn = line.split('\t')
                        inst_mn = [x.strip() for x in inst_mn]
                        pc_inst_map_arr.append(
                            [get_base_int_addr(inst_mn[0].replace(':', '')), #pc
                            inst_mn[2], # instruction mnemonic
                            ' '.join(inst_mn[2:]) # full instruction
                            ])

                    elif section == "data":
                        # outfile.write(line) # not annotating data section
                        prev_addr = get_base_int_addr(parts[0].strip())

                else: # no instruction/data in line
                    outfile.write(line)

            else: # not .text/.data section
                outfile.write(line)

        # write the last symbol
        if prev_addr:
            symbols[current_sym]['addr_end'] = prev_addr

    filter_str = []
    if section == "text":
        if args.pc_begin:
            filter_str.append(f"PC >= {args.pc_begin}")
            pc_begin = get_base_int_addr(args.pc_begin)
            symbols = {k: v for k, v in symbols.items()
                    if v['addr_start'] >= pc_begin}

        if args.pc_end:
            filter_str.append(f"PC <= {args.pc_end}")
            pc_end = get_base_int_addr(args.pc_end)
            symbols = {k: v for k, v in symbols.items()
                    if v['addr_end'] <= pc_end}

    sym_log = []
    for k,v in symbols.items():
        v['symbol_text'] = f"{k}"
        if section == "text":
            v['symbol_text'] += f" ({v['exec_count']})"

        sym_log.append(f"{num_to_hex(v['addr_start'], None)} - " + \
                       f"{num_to_hex(v['addr_end'], None)}: " + \
                       f"{v['symbol_text']}")

    print(f"Symbols found in {args.dasm} in '{section}' section:")
    if filter_str:
        print(f"Filtered by: {' and '.join(filter_str)}")
    for sym in sym_log[::-1]:
        print(sym)

    if args.save_symbols and section == "text":
        # convert to python types first
        symbols_py = {}
        for k,v in symbols.items():
            for k2,v2 in v.items():
                if isinstance(v2, np.int64):
                    v[k2] = int(v2)
            symbols_py[k] = dict(v)

        with open(os.path.join(logs_path, 'symbols.json'), 'w') as symfile:
            json.dump(symbols_py, symfile, indent=4)

    if section == "data":
        os.remove(outfile_name) # remove the dummy file
        return symbols, None # no df for data section

    df_out = pd.DataFrame(pc_inst_map_arr, columns=['pc', 'inst_mnm', 'inst'])
    return symbols, df_out

def add_cache_line_spans(ax) -> None:
    top = (int(ax.get_ylim()[1]) & ~0x3F) + CACHE_LINE_BYTES
    bottom = int(ax.get_ylim()[0]) & ~0x3F
    for i in range(bottom, top, CACHE_LINE_BYTES):
        color = 'k' if (i//CACHE_LINE_BYTES) % 2 == 0 else 'w'
        ax.axhspan(i, i+CACHE_LINE_BYTES, color=color, alpha=0.08, zorder=0)

    return ax

def annotate_chart(df, symbols, ax, args, ctype) -> \
plt.Axes:
    largs = {}
    if ctype == 'pc':
        largs = {'begin': args.pc_begin, 'end': args.pc_end,
                 'no_limit': args.no_pc_limit}
    elif ctype == 'dmem':
        largs = {'begin': args.dmem_begin, 'end': args.dmem_end,
                 'no_limit': args.no_dmem_limit}
    else:
        raise ValueError(f"Invalid chart type '{ctype}'. " + \
                         f"Available types are: 'pc', 'dmem'")

    #symbol_pos = ax.get_xlim()[1]
    symbol_pos = 1. # for transform=ax.get_yaxis_transform()

    # first apply execution limits
    if not largs['no_limit']:
        ax.set_ylim(top=(int(df[ctype].max()) & ~0x3F) + CACHE_LINE_BYTES)
        ax.set_ylim(bottom=int(df[ctype].min()) & ~0x3F)
    else:
        ax.set_ylim(bottom=0.0)

    ## then apply user limits
    if largs['begin']:
        ax.set_ylim(bottom=get_base_int_addr(largs['begin']))
    if largs['end']:
        ax.set_ylim(top=get_base_int_addr(largs['end']))

    start, end = 0, 0
    ymin, ymax = ax.get_ylim()
    # add lines for symbols, if any
    for k,v in symbols.items():
        start = v['addr_start']
        end = v['addr_end']
        # if the symbol is not in the range, skip it
        #if (start < ymin and end < ymin) or (start > ymax and end > ymax):
        if (start < ymin) or (start > ymax and end > ymax):
            continue
        ax.axhline(y=start, color='k', linestyle='-', alpha=0.5, lw=0.4)
        ax.text(symbol_pos, start,
                f" ^ {v['symbol_text']}", color='k',
                fontsize=9, ha='left', va='center',
                bbox=dict(facecolor='w', alpha=0.6, lw=0, pad=1),
                transform=ax.get_yaxis_transform())

    # add line for the last symbol, if any
    if symbols:
        # ends after last dmem entry, FIXME: should be the size of last inst
        ax.axhline(y=end+2, color='k', linestyle='-', alpha=0.5, lw=0.4)

    return ax

def draw_freq(df, hl_groups, title, symbols, args, ctype) -> plt.Figure:
    cols, ylabel = ctype_check(ctype)
    fig, ax = plt.subplots(figsize=(16,13), constrained_layout=True)

    rect_arr = []
    off = .25 if ctype == 'dmem' else .75
    for y, width, height in zip(df[cols[0]], df[cols[2]], df[cols[1]]):
        rect = patches.Rectangle((0, y+off), width, height-off, color='#649ac9')
        ax.add_patch(rect)
        rect_arr.append(rect)
    ax.set_xscale('log')

    if ctype == 'pc':
        # highlight specific instructions, if any
        hc = 0
        for hl_g in hl_groups:
            for i in range(len(df)):
                if df.iloc[i]['inst_mnm'] in hl_g:
                    rect_arr[i].set_color(hl_colors[hc])
            # add a dummy bar for the legend
            ax.barh(0, 0, color=hl_colors[hc], label=wrap_label(hl_g, 24))
            hc += 1

        if len(hl_groups) > 0:
            add_legend_for_hl_groups(ax, "trace")

    ax = annotate_chart(df, symbols, ax, args, ctype)
    ax = add_cache_line_spans(ax)

    # update axis
    #formatter = LogFormatter(base=10, labelOnlyBase=True)
    formatter = LogFormatterSciNotation(base=10)
    ax.xaxis.set_major_formatter(formatter)
    inc = find_loc_range(ax)
    ax.yaxis.set_major_locator(MultipleLocator(CACHE_LINE_BYTES*inc))
    ax.yaxis.set_major_formatter(FuncFormatter(num_to_hex))
    ax.set_xlim(left=0.5)
    ax.margins(y=0.01, x=0.1)

    # add a second x-axis
    ax_top = ax.twiny()
    ax_top.set_xlim(ax.get_xlim())
    ax_top.set_xscale('log')
    ax_top.xaxis.set_ticks_position('top')
    ax_top.xaxis.set_major_formatter(formatter)

    # label
    ax.set_xlabel("Count (log scale)")
    ax.set_ylabel(ylabel)
    ax.set_title(f"{ylabel} frequency profile for {title}")

    ax.grid(axis='x', linestyle='-', alpha=1, which='major')
    ax.grid(axis='x', linestyle='--', alpha=0.6, which='minor')

    return fig

def map_value(x, left_x, right_x, left_y, right_y):
    """
    Maps x from an input range [left_x, right_x]
    to an output value in [left_y, right_y] linearly.
    Values below left_x are clamped to left_y
    and values above right_x are clamped to right_y.

    Parameters:
        x (float): The input value.
        left_x (float): The lower bound of the input range.
        right_x (float): The upper bound of the input range.
        left_y (float): The output value for inputs <= left_x.
        right_y (float): The output value for inputs >= right_x.

    Returns:
        float: The mapped output value.
    """

    if x <= left_x: return left_y
    elif x >= right_x: return right_y

    # slope for linear interpolation.
    slope = (right_y - left_y) / (right_x - left_x)

    # interpolate
    return slope * (x - left_x) + left_y

def progressive_alpha(x):
    left_bound = 1_000
    right_bound = 100_000
    left_out = .7
    right_out = .1
    return map_value(x, left_bound, right_bound, left_out, right_out)

def progressive_lw(x):
    left_bound = 1_000
    right_bound = 100_000
    left_out = 1.5
    right_out = .3
    return map_value(x, left_bound, right_bound, left_out, right_out)

def plot_hw_hm(ax, df, col, hw_win_size):
    offset = .3
    hit = df[col].where(df[col] == 1, np.nan) + offset
    miss = df[col].where(df[col] == 0, np.nan) - offset
    alpha = progressive_alpha(df.index.size)
    ax.plot(df.smp, hit,
            ls='None', marker="|", ms=8, alpha=alpha, c='g', label='hit')
    ax.plot(df.smp, miss,
            ls='None', marker="|", ms=8, alpha=alpha, c='r', label='miss')

    # drop NaNs to have continuous mean
    running_avg = df[col] \
                  .dropna() \
                  .rolling(window=hw_win_size, min_periods=1) \
                  .mean()
    # reindex and forward fill to account for NaNs in the source
    running_avg = running_avg.reindex(df.index, method='ffill')
    lw = progressive_lw(df.index.size)
    ax.plot(df.smp, running_avg, lw=lw, color=(0,.3,.6,.7), label='avg')
    ax.set_ylim(-.8+offset, 1.8-offset)
    ax.set_yticks([0, .5, 1])
    ax.set_yticklabels(['0%', '', '100%'])
    #ax.legend(loc='upper right', ncol=3)

    metric = "ACC" if col == 'bp' else "HR"
    metric_mean = round(df[col].mean()*100,2)
    ax.text(1, 1,
            f"  {metric}: {metric_mean}%", color='k',
            fontsize=9, ha='left', va='center',
            bbox=dict(facecolor='w', alpha=0.6, lw=0, pad=1),
            transform=ax.get_yaxis_transform())

def draw_exec(df, hl_groups, title, symbols, args, ctype) -> plt.Figure:
    cols, ylabel = ctype_check(ctype)
    if len(df) > args.time_series_limit:
        print(f"Warning: too many PC entries to display in the time series " + \
              f"chart ({len(df)}). Limit is {args.time_series_limit} " + \
              f"entries. Either increase the limit or filter the data.")
        return None

    fig, ax = plt.subplots(ncols=1, nrows=5, figsize=(25,20),
                           sharex=True, height_ratios=[10, 1.2, 1.2, 1.2, 1],
                           constrained_layout=True)

    ax_t, ax_bp, ax_ic, ax_dc, ax_sp = ax
    ax_t.grid(axis='x', linestyle='-', alpha=.6, which='major')
    for a in ax:
        a.grid(axis='both', linestyle='-', alpha=.6, which='major')

    lw = progressive_lw(df.index.size)
    ax_sp.step(df.smp, df.sp_real, where='post', lw=lw, color=(0,.3,.6,.7))
    plot_hw_hm(ax_bp, df, 'bp', args.hw_win_size)
    plot_hw_hm(ax_ic, df, 'ic', args.hw_win_size)
    plot_hw_hm(ax_dc, df, 'dc', args.hw_win_size)

    # add PC/DMEM trace
    ax_t.step(df.smp, df[cols[0]], where='pre', lw=1.5, color=(0,.3,.6,.15))
    x_val, y_val = [], []
    for x, y, s in zip(df.smp, df[cols[0]], df[cols[1]]):
        x_val.extend([x, x, None]) # 'None' used to break the line
        y_val.extend([y, y + s, None])
    ax_t.plot(x_val, y_val, color='#649ac9', lw=2.)

    hc = 0
    if ctype == 'pc':
        trace_type = "Execution"
        hl_off = .15
        for hl_g in hl_groups:
            x_val_hl = []
            y_val_hl = []
            for inst in hl_g:
                df_hl = df[df['inst_mnm'] == inst]
                for x, y, s in zip(df_hl.smp, df_hl['pc'], df_hl['isz']):
                    x_val_hl.extend([x, x, None])
                    y_val_hl.extend([y+hl_off, y+s-hl_off, None])
            ax_t.plot(x_val_hl, y_val_hl, color=hl_colors[hc], alpha=1, lw=3)

            # add dummy scatter plot for the legend
            ax_t.scatter([], [], color=hl_colors[hc],
                         label=wrap_label(hl_g, 24))
            hc += 1

        if len(hl_groups) > 0:
            add_legend_for_hl_groups(ax_t, "trace")

    elif ctype == 'dmem':
        trace_type = "Data Memory"
        for hl_g in ['load', 'store']:
            x_val_hl = []
            y_val_hl = []
            df_hl = df[df['dtyp'] == hl_g]
            for x, y, s in zip(df_hl.smp, df_hl['dmem'], df_hl['dsz']):
                x_val_hl.extend([x, x, None])
                y_val_hl.extend([y, y+s, None])
            ax_t.plot(x_val_hl, y_val_hl, color=hl_colors[hc], lw=2)

            # add dummy scatter plot for the legend
            ax_t.scatter([], [], color=hl_colors[hc], label=hl_g)
            hc += 2 # skip yellow for visibility
            add_legend_for_hl_groups(ax_t, "trace")

    ax_t = annotate_chart(df, symbols, ax_t, args, ctype)
    ax_t = add_cache_line_spans(ax_t)

    ax_sp.text(1, df['sp_real'].max(),
               f"  SP max : {df['sp_real'].max()} bytes", color='k',
               fontsize=9, ha='left', va='center',
               bbox=dict(facecolor='w', alpha=0.6, lw=0, pad=1),
               transform=ax_sp.get_yaxis_transform())

    # update axis
    current_nbins = len(ax_t.get_xticks())
    locator = MaxNLocator(nbins=current_nbins*4, integer=True)
    ax_t.xaxis.set_major_locator(locator)
    ax_t.xaxis.set_major_formatter(FuncFormatter(to_k))
    inc = find_loc_range(ax_t)
    ax_t.yaxis.set_major_locator(MultipleLocator(CACHE_LINE_BYTES*inc))
    ax_t.yaxis.set_major_formatter(FuncFormatter(num_to_hex))
    ax_t.margins(y=0.03, x=0.01)

    # add a second x-axis
    ax_top = ax_t.twiny()
    ax_top.set_xlim(ax_t.get_xlim())
    ax_top.xaxis.set_major_locator(locator)
    ax_top.xaxis.set_ticks_position('top')
    ax_top.xaxis.set_major_formatter(FuncFormatter(to_k))

    # label
    ax_t.set_title(f"{trace_type} profile for {title}")
    ax_t.set_ylabel(ylabel)
    ax_sp.set_ylabel('Stack Pointer')
    ax_ic.set_ylabel('ICache')
    ax_dc.set_ylabel('DCache')
    ax_bp.set_ylabel('Branch\nPredictor')

    ax[-1].set_xlabel('Sample Count')

    return fig

def load_bin_trace(bin_log, args) -> pd.DataFrame:
    h = ['smp', # 64b - [0]
         'inst', 'pc', 'next_pc', 'dmem', # 128b - [1:4]
         'sp', # 32b - [5]
         'b_taken', 'ic', 'dc', 'bp', 'isz', 'dsz', # 48b - [6:11]
         'padding16', 'padding32'] # 48b [12:13]

    dtype = np.dtype([
        (h[ 0], np.uint64), # smp
        (h[ 1], np.uint32), # inst
        (h[ 2], np.uint32), # pc
        (h[ 3], np.uint32), # next_pc
        (h[ 4], np.uint32), # dmem
        (h[ 5], np.uint32), # sp
        (h[ 6], np.uint8 ), # taken
        (h[ 7], np.uint8 ), # ic
        (h[ 8], np.uint8 ), # dc
        (h[ 9], np.uint8 ), # bp
        (h[10], np.uint8 ), # isz
        (h[11], np.uint8 ), # dsz
        (h[12], np.uint16), # padding16
        (h[13], np.uint32), # padding32
    ])

    data = np.fromfile(bin_log, dtype=dtype)
    df = pd.DataFrame(data, columns=h)
    df = df.drop(columns=[c for c in h if 'padding' in c]) # drop padding cols

    # enum class hw_status_t { miss, hit, none };
    hw_status_t_none = 2
    df.ic = df.ic.replace(hw_status_t_none, np.nan)
    df.dc = df.dc.replace(hw_status_t_none, np.nan)
    df.bp = df.bp.replace(hw_status_t_none, np.nan)
    df['smp_taken'] = df['smp'].diff().fillna(df['smp']).astype(int)
    df['sp_real'] = BASE_ADDR + MEM_SIZE - df['sp']
    df['sp_real'] = df['sp_real'].apply(
        lambda x: 0 if x >= BASE_ADDR + MEM_SIZE else x)

    if args.save_converted_trace:
        dfs = df.copy()
        # some columns to hex
        for c in ["inst"]:
            dfs[c] = dfs[c].apply(lambda x: f'{x:08X}')
        for c in ["pc", "next_pc", "dmem", "sp"]:
            digits = int(math.ceil(np.log2(BASE_ADDR)/4))
            dfs[c] = dfs[c].apply(lambda x: f'{x:0{digits}X}')
        for c in ["isz", "dsz"]:
            dfs[c] = dfs[c].apply(lambda x: f'{x:01X}')
        dfs.to_csv(bin_log.replace('.bin', '.csv'), index=False)

    df_start = int(args.sample_begin) if args.sample_begin else 0
    df_end = int(args.sample_end) if args.sample_end else df.smp.max()
    df = df.loc[df['smp'].between(df_start,df_end)]

    return df

def run_bin_trace(bin_log, hl_groups, title, args) -> \
Tuple[pd.DataFrame, plt.Figure, plt.Figure]:
    df = load_bin_trace(bin_log, args)
    df_out = None
    dict_out = {}
    if not args.dmem_only:
        df_out, fig_pc, fig2_pc = run_bin_trace_pc(df, hl_groups, title, args)
        dict_out['pc'] = fig_pc
        dict_out['pc_exec'] = fig2_pc
    if not args.pc_only:
        _, fig_dmem, fig2_dmem = run_bin_trace_dmem(df, hl_groups, title, args)
        dict_out['dmem'] = fig_dmem
        dict_out['dmem_exec'] = fig2_dmem

    return df_out, dict_out

def run_bin_trace_pc(df_og, hl_groups, title, args) -> \
Tuple[pd.DataFrame, plt.Figure, plt.Figure]:

    df = df_og.groupby('pc').agg(
        isz=('isz', 'first'), # get only the first value
        count=('smp_taken', 'sum') # count them all (size of the group)
    ).reset_index()
    df = df.sort_values(by='pc', ascending=True)
    df['pc'] = df['pc'].astype(int)
    df['pc_hex'] = df['pc'].apply(lambda x: f'{x:08x}')

    symbols = {}
    m_hl_groups = []
    if args.dasm:
        m_hl_groups = hl_groups
        symbols, df_map = backannotate_dasm(args, df, "text")
        # merge df_map into df by keeping only records in the df
        df = pd.merge(df, df_map, how='left', left_on='pc', right_on='pc')
        df_og = pd.merge(df_og, df_map, how='left', left_on='pc', right_on='pc')

    if args.symbols_only:
        return df, None, None

    fig = draw_freq(df, m_hl_groups, title, symbols, args, 'pc')
    fig2 = draw_exec(df_og, m_hl_groups, title, symbols, args, 'pc')
    return df, fig, fig2

def run_bin_trace_dmem(df_og, hl_groups, title, args) -> \
Tuple[pd.DataFrame, plt.Figure, plt.Figure]:

    df_og['dtyp'] = df_og['dsz'].apply(lambda x: 'store' if x >= 8 else 'load')
    df_og['dsz'] = df_og['dsz'].apply(lambda x: x-8 if x >= 8 else x)
    df_og['dmem'] = df_og['dmem'].replace(0, np.nan) # gaps in dmem acces/chart

    # isa is byte addressable, expand each access to a single byte
    exp_rows = []
    for i, row in df_og.iterrows():
        addr, sz = row['dmem'], row['dsz']
        if sz > 1:
            for i in range(sz):
                exp_rows.append({'dmem': addr+i, 'dsz': 1, 'dtyp': row['dtyp']})
        else:
            exp_rows.append({'dmem': addr, 'dsz': sz, 'dtyp': row['dtyp']})

    df_exp = pd.DataFrame(exp_rows)
    df = df_exp.groupby('dmem').agg(
        dsz=('dsz', 'first'), # get only the first value
        count=('dmem', 'size') # count them all (size of the group)
    ).reset_index()
    df = df.sort_values(by='dmem', ascending=True)
    df['dmem'] = df['dmem'].astype(int)
    df['dmem_hex'] = df['dmem'].apply(lambda x: f'{x:08x}')
    if df.iloc[0]['dmem'] == 0:
        df = df.drop(0)

    symbols = {}
    if args.dasm:
        symbols, _ = backannotate_dasm(args, df, "data")

    if args.symbols_only:
        return df, None, None

    fig = draw_freq(df, [], title, symbols, args, ctype='dmem')
    fig2 = draw_exec(df_og, [], title, symbols, args, 'dmem')
    return df, fig, fig2

def parse_args() -> argparse.Namespace:
    TIME_SERIES_LIMIT = 50000
    parser = argparse.ArgumentParser(description="Analysis of memory access logs and traces")
    # either
    parser.add_argument('-i', '--inst_log', type=str, help="Path to JSON instruction count log with profiling data")
    # or
    parser.add_argument('-t', '--trace', type=str, help="Path to binary execution trace")

    # instruction count log only options
    parser.add_argument('--exclude', type=inst_exists, nargs='+', help="Exclude specific instructions. Instruction count log only option")
    parser.add_argument('--exclude_type', type=inst_type_exists, nargs='+', help=f"Exclude specific instruction types. Available types are: {', '.join(inst_t.keys())}. Instruction count log only option")
    parser.add_argument('--top', type=int, help="Number of N most common instructions to display. Default is all. Instruction count log only option")
    parser.add_argument('--allow_zero', action='store_true', default=False, help="Allow instructions with zero count to be displayed. Instruction count log only option")

    # trace only options
    parser.add_argument('--dasm', type=str, help="Path to disassembly 'dasm' file to backannotate the trace. This file and the new annotated file is saved in the same directory as the input trace with *.prof.<original ext> suffix. Trace only option")

    parser.add_argument('--sample_begin', type=str, help="Show only samples after this sample. Applied before any other filtering options. Trace only option")
    parser.add_argument('--sample_end', type=str, help="Show only samples before this sample. Applied before any other filtering options. Trace only option")

    parser.add_argument('--no_pc_limit', action='store_true', help="Don't limit the PC range to the execution trace. Useful when logging is done with HINT instruction. By default, the PC range is limited to the execution trace range. Trace only option")
    parser.add_argument('--pc_begin', type=str, help="Show only PCs after this PC. Input is a hex string. E.g. '0x80000094'. Applied after --no_pc_limit. Trace only option")
    parser.add_argument('--pc_end', type=str, help="Show only PCs before this PC. Input is a hex string. E.g. '0x800000ec'. Applied after --no_pc_limit. Trace only option")
    parser.add_argument('--pc_only', action='store_true', help="Only backannotate and display the PC trace. Trace only option")

    parser.add_argument('--no_dmem_limit', action='store_true', help="Don't limit the DMEM range to the execution trace. Same as --no_pc_limit but for DMEM. Trace only option")
    parser.add_argument('--dmem_begin', type=str, help="Show only DMEM addresses after this address. Input is a hex string. E.g. '0x80000200'. Applied after --no_dmem_limit. Trace only option")
    parser.add_argument('--dmem_end', type=str, help="Show only DMEM addresses before this address. Input is a hex string. E.g. '0x800002f0'. Applied after --no_dmem_limit. Trace only option")
    parser.add_argument('--dmem_only', action='store_true', help="Only backannotate and display the DMEM trace. Trace only option")

    parser.add_argument('--hw_win_size', type=int, default=16, help="Number of samples to use for rolling average window for branch predictor accuracy and caches hit rate. Trace only option")
    parser.add_argument('--symbols_only', action='store_true', help="Only backannotate and display the symbols found in the 'dasm' file. Requires --dasm. Doesn't display figures and ignores all save options except --save_csv. Trace only option")
    parser.add_argument('--save_symbols', action='store_true', help="Save the symbols found in the 'dasm' file as a JSON file. Requires --dasm. Trace only option")
    parser.add_argument('--time_series_limit', type=int, default=TIME_SERIES_LIMIT, help=F"Limit the number of address entries to display in the time series chart. Default is {TIME_SERIES_LIMIT}. Trace only option")
    parser.add_argument('--save_converted_trace', action='store_true', help="Save the converted binary trace as a CSV file. Trace only option")

    # common options
    parser.add_argument('--highlight', '--hl', type=str, nargs='+', help="Highlight specific instructions. Multiple instructions can be provided as a single string separated by whitespace (multiple groups) or separated by commas (multiple instructions in a group). E.g.: 'add,addi sub' colors 'add' and 'addi' the same and 'sub' a different color.")
    # TODO: add highlighting for function calls/PC ?
    parser.add_argument('-s', '--silent', action='store_true', help="Don't display chart(s) in pop-up window")
    parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
    parser.add_argument('--save_svg', action='store_true', help="Save charts as SVG")
    parser.add_argument('--save_csv', action='store_true', help="Save source data formatted as CSV")

    return parser.parse_args()

def run_main(args) -> None:
    run_inst = args.inst_log is not None
    run_trace = args.trace is not None

    if run_inst and run_trace:
        raise ValueError("Inst and trace options cannot be mixed")

    if not run_inst and not run_trace:
        raise ValueError(
            "No JSON instruction count log or execution trace provided")

    if (args.symbols_only or args.save_symbols) and run_inst:
        raise ValueError("--symbols_only cannot be used with instruction logs")

    if (args.symbols_only or args.save_symbols) and not args.dasm:
        raise ValueError("--symbols_only requires --dasm")

    if (args.pc_only and args.dmem_only and not args.symbols_only):
        raise ValueError("--pc_only and --dmem_only cannot be used together")

    # filtering args
    if args.sample_begin or args.sample_end or \
       args.pc_begin or args.pc_end or \
       args.dmem_begin or args.dmem_end:
        if not args.trace:
            raise ValueError("trace filtering options require execution trace")
        if not args.dasm:
            raise ValueError("trace filtering options require --dasm")

    # check sample filtering
    if (args.sample_begin and args.sample_end):
        if int(args.sample_begin) >= int(args.sample_end):
            raise ValueError("--sample_begin must be less than --sample_end")

    # check PC filtering
    if (args.pc_begin and args.pc_end):
        if int(args.pc_begin, 16) >= int(args.pc_end, 16):
            raise ValueError("--pc_begin must be less than --pc_end")

    # check DMEM filtering
    if (args.dmem_begin and args.dmem_end):
        if int(args.dmem_begin, 16) >= int(args.dmem_end, 16):
            raise ValueError("--dmem_begin must be less than --dmem_end")

    args_log = args.trace if run_trace else args.inst_log
    if not os.path.exists(args_log):
        raise FileNotFoundError(f"File {args_log} not found")

    hl_groups = []
    if not (args.highlight == None):
        if len(args.highlight) > len(hl_colors):
            raise ValueError(f"Too many instructions to highlight " + \
                             f"max is {len(hl_colors)}, " + \
                             f"got {len(args.highlight)}")
        else:
            if len(args.highlight) == 1: # multiple arguments but 1 element
                hl_groups = args.highlight[0].split()
            else: # already split on whitespace (somehow?)
                hl_groups = args.highlight
            hl_groups = [ah.split(",") for ah in hl_groups]

    fig_arr = []
    ext = os.path.splitext(args_log)[1]
    title = os.path.basename(os.path.dirname(args_log)).replace("out_", "")
    log_path = os.path.realpath(args_log)

    if run_trace:
        df, figs_dict = run_bin_trace(args_log, hl_groups, title, args)
        for name, fig in figs_dict.items():
            fig_arr.append([log_path.replace(ext, f"_{name}{ext}"), fig])
    else:
        df, fig = run_inst_log(args_log, hl_groups, title, args)
        fig_arr.append([log_path, fig])
        figs_dict = {"inst": fig}

    if not args.silent:
        plt.show(block=False)
        if not is_notebook():
            input("Press Enter to close all plots...")
    plt.close('all')

    if args.save_csv:
        df.to_csv(args_log.replace(ext, "_out.csv"), index=False)

    if args.symbols_only and run_trace:
        return

    if args.save_png: # each chart is saved as a separate PNG file
        for name, fig in (fig_arr):
            fig.savefig(name.replace(" ", "_").replace(ext, ".png"))

    if args.save_svg: # each chart is saved as a separate SVG file
        for name, fig in (fig_arr):
            fig.savefig(name.replace(" ", "_").replace(ext, ".svg"))

if __name__ == "__main__":
    args = parse_args()
    run_main(args)
