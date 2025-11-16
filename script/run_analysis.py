#!/usr/bin/env python3

import argparse
import json
import math
import os
import textwrap
from typing import Any, Dict, List, Tuple

import matplotlib
import matplotlib.patches as patches
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.ticker import (AutoMinorLocator, EngFormatter, FixedLocator,
                               FuncFormatter, LinearLocator, LogFormatter,
                               LogFormatterSciNotation, MaxNLocator,
                               MultipleLocator)
from matplotlib.widgets import RangeSlider
from utils import FMT_AXIS, get_test_title, is_notebook, smarter_eng_formatter

#SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))

CACHE_LINE_BYTES = 64
BASE_ADDR = 0x40000
MEM_SIZE = 65536

FS_EXEC_X = 24.7
FS_EXEC_Y = 13
GS_EXEC = {'top': .95, 'bottom': .1, 'left': .09, 'right': .8, 'hspace': .05}
TRACE_TYPE = ["Execution", "Data"]

TRACE_NA = 255

FREQ_DEFAULT = 100 # MHz

# TODO: add function calls/graphs highlight when supported by the profiler
CLR = {
    "blue_b0": "#0077b6",
    "blue_l1": "#00b4d8",
    "blue_l2": "#90e0ef",
}

CLR_HL = [
    "#3ECCBB", # turquoise
    "#EED595", # peach yellow
    "#f4a261", # sandy brown
    "#c03a2d", # persian red
    "#457B74", # myrtle green
    "#b17acf", # lavander (floral)
]

CLR_TAB_BLUE = "tab:blue"

class icfg:
    ALU = "ALU"
    MUL = "MUL"
    DIV = "DIV/REM"
    BITMANIP = "BITMANIP"
    SIMD = "SIMD"
    SIMD_DOT = "SIMD_DOT"
    SIMD_ADD = "SIMD_ADD"
    SIMD_MUL = "SIMD_MUL"
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

    INST_T_MEM = {
        MEM_S: ["sb", "sh", "sw", "c.swsp", "c.sw"],
        MEM_L: ["lb", "lh", "lw", "lbu", "lhu", "c.lwsp", "c.lw", "c.li"],
    }

    INST_T_SIMD = {
        SIMD_ADD: ["add16", "add8", "sub16", "sub8"],
        SIMD_MUL: ["mul16", "mul16u", "mul8", "mul8u"],
        SIMD_DOT: ["dot16", "dot8", "dot4"],
    }

    INST_T = {
        ALU: [
            "add", "sub", "sll", "srl", "sra", "slt", "sltu", "xor", "or","and",
            "addi", "slli", "srli", "srai", "slti", "sltiu","xori","ori","andi",
            "lui", "auipc",
            "c.add", "c.addi", "c.addi16sp", "c.addi4spn", "c.sub",
            "c.andi", "c.srli", "c.slli", "c.srai", "c.and", "c.xor", "c.or",
            "c.mv",
            "c.lui",
        ],
        MUL: ["mul", "mulh", "mulsu", "mulu"],
        DIV: ["div", "divu", "rem", "remu"],
        BITMANIP: ["max", "maxu", "min", "minu"],
        SIMD: INST_T_SIMD[SIMD_ADD] + \
                INST_T_SIMD[SIMD_MUL] + \
                INST_T_SIMD[SIMD_DOT],
        UNPAK: [
            "unpk16", "unpk16u", "unpk8", "unpk8u",
            "unpk4", "unpk4u", "unpk2", "unpk2u",
        ],
        MEM: INST_T_MEM[MEM_S] + INST_T_MEM[MEM_L],
        MEM_HINTS: ["scp.ld", "scp.rel"],
        BRANCH: ["beq", "bne", "blt", "bge", "bltu", "bgeu", "c.beqz","c.bnez"],
        JUMP: ["jalr", "jal","c.j" ,"c.jal" ,"c.jr" ,"c.jalr"],
        CSR: ["csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"],
        ENV: ["ecall", "ebreak", "c.ebreak"],
        NOP: ["nop", "c.nop"],
        FENCE: ["fence", "fence.i"],
    }

    INST_T_SIMD_Z = INST_T[SIMD] + INST_T[UNPAK]
    OPS_PER_INST = {
        1: INST_T[ALU]+INST_T[MEM]+INST_T[MUL]+INST_T[DIV]+INST_T[BITMANIP],
        2: [s for s in INST_T_SIMD_Z if ("16" in s and 'dot' not in s)],
        3: ['dot16'], # 2x mul, 1x sum
        4: [s for s in INST_T_SIMD_Z if ("8" in s and 'dot' not in s)],
        7: ['dot8'], # 4x mul, 3x sum
        8: [s for s in INST_T_SIMD_Z if ("4" in s and 'dot' not in s)],
        15: ['dot4'], # 8x mul, 7x sum
        16: [s for s in INST_T_SIMD_Z if ("2" in s and 'dot' not in s)],
    }

    BRANCH_DENSITY = { 1: INST_T[BRANCH] + INST_T[JUMP] }

    ALL_INST = np.concatenate(list(INST_T.values())).tolist()
    ALL_INST_TYPES = list(INST_T.keys())

    # create a reverse map for instruction types
    INST_TO_TYPE_DICT = {}
    for k, v in INST_T.items():
        for inst in v:
            INST_TO_TYPE_DICT[inst] = k

    INST_TO_MEM_TYPE_DICT = {}
    for k, v in INST_T_MEM.items():
        for inst in v:
            INST_TO_MEM_TYPE_DICT[inst] = k

    # create a reverse map for ops per instruction
    INST_TO_OPS_DICT = {}
    for k, v in OPS_PER_INST.items():
        for inst in v:
            INST_TO_OPS_DICT[inst] = k

    INST_TO_BD_DICT = {}
    for k, v in BRANCH_DENSITY.items():
        for inst in v:
            INST_TO_BD_DICT[inst] = k

    # groups of instructions to highlight by default
    HL_DEFAULT = [
        INST_T[MEM],
        INST_T[BRANCH],
        INST_T[JUMP],
        INST_T[MUL] + INST_T[DIV],
        INST_T[SIMD],
        INST_T[UNPAK]
    ]

    HL_COLORS_OPS = {
        ALU: CLR_TAB_BLUE,
        MEM: CLR_HL[0], # turquoise
        MUL: CLR_HL[3], # persian red
        DIV: CLR_HL[2], # sandy brown
        SIMD: CLR_HL[4], # myrtle green
        UNPAK: CLR_HL[5], # lavander (floral)
        # separate chart
        BRANCH: CLR_HL[0], # peach yellow
        JUMP: CLR_HL[3], # persian red
    }

    # memory instructions breakdown store vs load
    INST_MEM_BD = {
        MEM_L : ["Load", CLR["blue_l1"]],
        MEM_S : ["Store", CLR["blue_l2"]],
    }

# common functions
def hex2int(addr) -> int:
    return int(addr,16) # - int(BASE_ADDR)

def int2hex(val, pos) -> str:
    return f"0x{int(val):04X}"

def get_count(parts, df) -> Tuple[int, int]:
    pc = hex2int(parts[0].strip())
    count_series  = df.loc[df["pc"] == pc, "count"]
    count = count_series.squeeze() if not count_series.empty else 0
    return int(count), pc # FIXME: count may be float in rtl trace?

def get_title(src, typ, wl) -> str:
    return f"{src} trace - {typ} profile for '{wl}'"

def inst_exists(inst) -> str:
    if inst not in icfg.ALL_INST:
        raise ValueError(
            f"Invalid instruction '{inst}'. " + \
            f"Available instructions are: {', '.join(icfg.ALL_INST)}")
    return inst

def inst_type_exists(inst_type) -> str:
    if inst_type not in icfg.ALL_INST_TYPES:
        raise ValueError(
            f"Invalid instruction type '{inst_type}'. " + \
            f"Available types are: {', '.join(icfg.ALL_INST_TYPES)}")
    return inst_type

def ctype_check(ctype:str) -> Tuple[List[str], str]:
    if ctype == 'pc':
        cols = ['pc', 'isz', 'count']
        ylabel = "Program Counter"
    elif ctype == 'dmem':
        cols = ['dmem', 'dsz', 'count']
        ylabel = "Data"
    else:
        raise ValueError(f"Invalid chart type '{ctype}'. " + \
                         f"Available types are: 'pc', 'dmem'")

    return cols, ylabel

def exec_df_within_limits(df_len, limit) -> bool:
    if df_len > limit:
        print(f"Warning: too many entries to display in the time series " + \
              f"chart: {df_len}. Limit is {limit} entries. " + \
              f"Either increase the limit or filter the data.")
        return False
    return True

def rolling_mean(data: pd.Series, win_size: int) -> pd.Series:
    """
    Moving average with zero-padding BEFORE the signal begins.
    Equivalent to prepending `win_size` zeros but implemented efficiently.
    """
    x = data.to_numpy()
    pad = np.zeros(win_size, dtype=x.dtype)
    padded = np.concatenate((pad, x))
    c = padded.cumsum()

    # moving average:
    #   avg[i] = (c[i+win_size] - c[i]) / win_size
    # this gives the mean over padded[i : i+win_size]
    avg = (c[win_size:] - c[:-win_size]) / win_size

    return pd.Series(avg, index=data.index).round(3) # as series with og index

# common functions for plotting
def find_loc_range(ax) -> int:
    ymin, ymax = ax.get_ylim()
    yrange = ymax - ymin
    # find the integer power of 2 at or smaller than the range in KB
    return 2**int(math.log2(max(1, yrange//1024)))

def wrap_text(arr:List[str], max_len:int) -> str:
    label = ', '.join(arr)
    wrapped = '\n'.join(textwrap.wrap(label, max_len))
    return wrapped

def add_legend_for_hl_groups(ax, chart_type:str) -> None:
    title = "Highlighted\nInstructions\n"
    if chart_type == "log":
        title = title.replace("\n", " ").strip()
        ax.legend(loc='lower right', title=title, framealpha=0.5)

    # put legend in the top right corner, outside the plot
    elif chart_type == "trace_freq":
        ax.legend(title=title, framealpha=0.5, loc='upper right',
                  bbox_to_anchor=(1.68, 1.03), borderaxespad=0.)
    elif chart_type == "trace_exec":
        ax.legend(title=title, framealpha=0.5, loc='upper right',
                  bbox_to_anchor=(1.27, 1.03), borderaxespad=0.)

    else:
        raise ValueError(f"Invalid chart type '{chart_type}'")

def add_outside_legend(ax, title:str) -> None:
    ax.legend(
        loc='upper left', ncol=1, fontsize=9, title=title,
        bbox_to_anchor=(1.005, 1.03), borderaxespad=0.
    )

def plot_dummy_line(ax, color, lw, label) -> None:
    # used when legend lw needs to be decoupled from the actual line lw
    ax.plot([], [], color=color, lw=lw, label=label)

def map_value(x, left_x, right_x, left_y, right_y) -> float:
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

def progressive_alpha(x) -> float:
    left_bound = 1_000
    right_bound = 100_000
    left_out = .7
    right_out = .1
    return map_value(x, left_bound, right_bound, left_out, right_out)

def progressive_lw(x) -> float:
    left_bound = 1_000
    right_bound = 1_000_000
    left_out = 1.5
    right_out = .2
    return map_value(x, left_bound, right_bound, left_out, right_out)

def add_cache_line_spans(ax) -> plt.Axes:
    top = (int(ax.get_ylim()[1]) & ~0x3F) + CACHE_LINE_BYTES
    bottom = int(ax.get_ylim()[0]) & ~0x3F
    for i in range(bottom, top, CACHE_LINE_BYTES):
        if (i//CACHE_LINE_BYTES) % 2 != 0:
            # color every other line in gray
            continue
        ax.axhspan(i, i+CACHE_LINE_BYTES, color='k', alpha=0.05, zorder=0)

    return ax

def add_spans(ax) -> plt.Axes:
    top = int(ax.get_ylim()[1])
    bottom = int(ax.get_ylim()[0])
    for i in range(bottom, top+1, 2):
        ax.axhspan(i-.5, i+.5, color='k', alpha=0.03, zorder=0)
    return ax

def annotate_chart(df, symbols, ax, args, ctype) -> \
plt.Axes:
    largs = {}
    if ctype == 'pc':
        largs = {
            'begin': args.pc_begin,
            'end': args.pc_end,
            'no_limit': args.no_pc_limit
        }
    elif ctype == 'dmem':
        largs = {
            'begin': args.dmem_begin,
            'end': args.dmem_end,
            'no_limit': args.no_dmem_limit
        }
    else:
        raise ValueError(f"Invalid chart type '{ctype}'. " + \
                         f"Available types are: 'pc', 'dmem'")

    #symbol_pos = ax.get_xlim()[1]
    symbol_pos = 1.005 # used for transform=ax.get_yaxis_transform()

    # visual only, apply execution limits
    if largs['no_limit']:
        ax.set_ylim(bottom=0.0)
    else:
        # FIXME: temp workaround for empty rtl dmem profile
        if not df[ctype].dropna().empty:
            ax.set_ylim(
                top=(int(df[ctype].dropna().max()) & ~0x3F) + CACHE_LINE_BYTES)
            ax.set_ylim(bottom=int(df[ctype].min()) & ~0x3F)

    # visual only, align with specified limits, if any
    if largs['begin']:
        ax.set_ylim(bottom=hex2int(largs['begin']))
    if largs['end']:
        ax.set_ylim(top=hex2int(largs['end']))

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
        ax.axhline(y=start, color='k', linestyle='--', alpha=0.7, lw=0.5)
        txt = ax.text(
            symbol_pos, start,
            f" ^ {v['symbol_text']}", color='k',
            fontsize=9, ha='left', va='center',
            bbox=dict(facecolor='w', alpha=0.6, lw=0, pad=1),
            transform=ax.get_yaxis_transform(),
            clip_on=False, # draw outside axes
        )
        # don't let constrained_layout/tight_layout expand spacing
        txt.set_in_layout(False)

    # add line for the last symbol, if any
    if symbols:
        # ends after last dmem entry, FIXME: should be the size of last inst
        ax.axhline(y=end+4, color='k', linestyle='-', alpha=0.7, lw=0.5)

    return ax

def draw_inst_profile(df, hl_inst_g, title, args) -> plt.Figure:
    inst_profiled = df['count'].sum()
    if inst_profiled == 0:
        raise ValueError("No instructions in the json log")
    df['i_type'] = df['name'].map(icfg.INST_TO_TYPE_DICT)
    df['i_mem_type'] = df['name'].map(icfg.INST_TO_MEM_TYPE_DICT)
    df['i_ops'] = df['name'].map(icfg.INST_TO_OPS_DICT)
    df['ops'] = df['count'] * df['i_ops']

    # filter out instructions if needed
    if args.exclude:
        df = df[~df['name'].isin(args.exclude)]

    if args.exclude_type:
        df = df[~df['i_type'].isin(args.exclude_type)]

    if args.top:
        df = df.tail(args.top)
    if not args.allow_zero:
        df = df[df['count'] != 0]

    # separate the instructions by type for count
    df_g = df[['i_type', 'count']].groupby('i_type').sum()
    df_g = df_g.sort_values(by='count', ascending=True)

    # separate the memory instructions by type for count
    df_mem_g = df[['i_mem_type', 'count']].groupby('i_mem_type').sum()
    df_mem_g = df_mem_g.sort_values(by='count', ascending=False)

    # separate the instructions by type for ops
    df_ops_g = df[['i_type', 'ops']].groupby('i_type').sum()
    df_ops_g = df_ops_g.sort_values(by='ops', ascending=True)
    if not args.allow_zero:
        df_ops_g = df_ops_g[df_ops_g['ops'] != 0]

    # add a bar chart
    fmt = smarter_eng_formatter()
    ldf = [len(df), len(df_g), len(df_ops_g)]
    ROWS, COLS = 3, 1
    box = []
    fig, ax = plt.subplots(
        ROWS, COLS, figsize=(COLS*10, ROWS*(sum(ldf)/10)+2),
        height_ratios=ldf, constrained_layout=True)
    suptitle_str = f"Execution profile for '{title}'"
    suptitle_str += f"\nSamples profiled: {fmt(inst_profiled)}"
    if args.exclude or args.exclude_type:
        suptitle_str += f" ({df['count'].sum()} shown, "
        suptitle_str += f"{inst_profiled - df['count'].sum()} excluded)"

    fig.suptitle(suptitle_str, size=12)
    box.append(
        ax[0].barh(df['name'], df['count'], color=CLR["blue_b0"]))
    box.append(
        ax[1].barh(df_g.index, df_g['count'], color=CLR["blue_b0"]))
    box.append(
        ax[2].barh(df_ops_g.index, df_ops_g['ops'], color=CLR["blue_b0"]))

    y_ax0_offset = min(0.025, len(df)/2_000)
    ax[0].margins(y=0.03-y_ax0_offset)
    ax[1].margins(y=0.03)
    ax[2].margins(y=0.03)

    y_labels = ["Instruction", "Instruction Type", "Operation"]
    for i in range(ROWS):
        bars = box[i]
        # sum of all bar values on that axis
        total = sum(bar.get_width() for bar in bars)
        if i == 2: # ops don't have %, can't compare them like that
            labels = [f"{fmt(bar.get_width())}" for bar in bars]
        else:
            labels = [
                f"{fmt(bar.get_width())} ({bar.get_width() / total:.1%})"
                for bar in bars]
        ax[i].bar_label(bars, labels=labels, padding=3, fmt=fmt)
        ax[i].set_xlabel('Count')
        ax[i].set_ylabel(y_labels[i])
        ax[i].grid(axis='x')
        ax[i].margins(x=0.15)
        ax[i].xaxis.set_major_formatter(FMT_AXIS)

    # highlight specific instructions, if any
    hc = 0
    for hl_g in hl_inst_g:
        for i, r in enumerate(box[0]):
            if df.iloc[i]['name'] in hl_g:
                r.set_color(CLR_HL[hc])
        ax[0].barh(0, 0, color=CLR_HL[hc], label=', '.join(hl_g))
        hc += 1

    if len(hl_inst_g) > 0:
        add_legend_for_hl_groups(ax[0], "log")

    # add memory instructions breakdown, if any
    df_mem_g = df_mem_g[df_mem_g['count'] != 0] # never label if count is zero
    if len(df_mem_g) > 0:
        mem_type_index = df_g.index.get_loc("MEM")
        left_start = 0
        for i, row in df_mem_g.iterrows():
            rect_m = ax[1].barh(mem_type_index, row['count'], left=left_start,
                                label=icfg.INST_MEM_BD[row.name][0],
                                color=icfg.INST_MEM_BD[row.name][1])
            ax[1].bar_label(rect_m, padding=0, label_type='center', size=7)
            left_start += row['count']

        ax[1].legend(loc='lower right')

    return fig

# dasm
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
    if section == "text":
        PADDING = len(str(int(df['count'].max()))) + 1

    outfile_name = dasm_name.replace(dasm_ext, new_dasm_ext)
    outfile_name = os.path.join(logs_path, outfile_name)
    with open(args.dasm, 'r') as infile, open(outfile_name, 'w') as outfile:
        current_sym = None
        append = False
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
                    addr_start = hex2int(parts[0].strip())
                    symbol_name = parts[1][1:-1] # remove <> from symbol name

                    if prev_addr == addr_start:
                        continue # skip duplicate symbol

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
                            [hex2int(inst_mn[0].replace(':', '')), #pc
                            inst_mn[2], # instruction mnemonic
                            ' '.join(inst_mn[2:]) # full instruction
                            ])

                    elif section == "data":
                        # outfile.write(line) # not annotating data section
                        prev_addr = hex2int(parts[0].strip())

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
            pc_begin = hex2int(args.pc_begin)
            symbols = {k: v for k, v in symbols.items()
                    if v['addr_start'] >= pc_begin}

        if args.pc_end:
            filter_str.append(f"PC <= {args.pc_end}")
            pc_end = hex2int(args.pc_end)
            symbols = {k: v for k, v in symbols.items()
                    if v['addr_end'] <= pc_end}

    fmt = smarter_eng_formatter()
    sym_log = []
    for k,v in symbols.items():
        v['symbol_text'] = f"{k}"
        if section == "text":
            v['symbol_text'] += f" ({fmt(v['exec_count'])})"

        sym_log.append(f"{int2hex(v['addr_start'], None)} - " + \
                       f"{int2hex(v['addr_end'], None)}: " + \
                       f"{v['symbol_text']}")

    if args.print_symbols:
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

        sym_json = os.path.join(logs_path, 'symbols.json')
        with open(sym_json, 'w') as symfile:
            json.dump(symbols_py, symfile, indent=4)
        print(f"Symbols saved in '{sym_json}'")

    if section == "data":
        os.remove(outfile_name) # remove the dummy file
        return symbols, None # no df for data section

    df_out = pd.DataFrame(
        pc_inst_map_arr, columns=['pc', 'inst_mnm', 'inst_asm'])
    return symbols, df_out

# sliders
def register_true_home(fig) -> None:
    tb = getattr(fig.canvas.manager, "toolbar", None)
    if tb is None:
        return

    # snapshot current view limits (skip slider axes)
    homes = [(ax, ax.get_xlim(), ax.get_ylim())
             for ax in fig.axes if not getattr(ax, "_is_slider_ax", False)]

    def _home(*_):
        # push the pre-Home view so Back goes here
        tb.push_current()
        # apply Home to all plot axes
        for ax, xlim, ylim in homes:
            ax.set_xlim(*xlim)
            ax.set_ylim(*ylim)
        fig.canvas.draw_idle()
        # push the post-Home view so Forward goes here
        tb.push_current()

    tb.home = _home

def attach_xrange_slider(ax, ax_top, gap=0.03, h=0.035) -> RangeSlider:
    fig = ax.figure
    pos = ax.get_position()
    slider_ax = fig.add_axes([pos.x0, pos.y0 - gap - h, pos.width, h])
    slider_ax._is_slider_ax = True

    xmin, xmax = ax.get_xlim()
    rs = RangeSlider(
        ax=slider_ax,
        label="",
        valmin=float(min(xmin, xmax)),
        valmax=float(max(xmin, xmax)),
        valinit=(float(min(xmin, xmax)), float(max(xmin, xmax))),
        handle_style=dict(size=1, facecolor="w", edgecolor="w"),
    )

    # ticks copied from main X (snapshot)
    slider_ax.set_axis_on()
    slider_ax.yaxis.set_visible(False)
    slider_ax.xaxis.set_visible(True)
    slider_ax.set_xlim(rs.valmin, rs.valmax)
    abs_ticks = ax.get_xticks()
    slider_ax.xaxis.set_major_locator(FixedLocator(abs_ticks))
    slider_ax.xaxis.set_major_formatter(ax.xaxis.get_major_formatter())

    # spines: keep bottom only
    for s in ("top", "left", "right"):
        slider_ax.spines[s].set_visible(False)
    slider_ax.spines["bottom"].set_visible(True)
    slider_ax.tick_params(axis="x", pad=1, length=3)

    # colors
    rs.poly.set_facecolor("white")
    rs.poly.set_alpha(0.8)
    rs.track.set_facecolor("black")
    rs.track.set_alpha(0.4)

    # handles
    barL = slider_ax.axvline(rs.valmin, 0, 1, linewidth=6, color="k")
    barR = slider_ax.axvline(rs.valmax, 0, 1, linewidth=6, color="k")

    # sync
    _sync = {"slider": False, "axes": False}

    def _on_slider(_):
        if _sync["axes"]: return
        _sync["slider"] = True
        lo, hi = rs.val
        ax.set_xlim(lo, hi)
        ax_top.set_xlim(lo, hi)
        barL.set_xdata([lo, lo])
        barR.set_xdata([hi, hi])
        fig.canvas.draw_idle()
        _sync["slider"] = False

    rs.on_changed(_on_slider)

    def _on_xlim_changed(event_ax):
        if _sync["slider"]: return
        _sync["axes"] = True
        lo, hi = event_ax.get_xlim()
        rs.set_val((lo, hi))
        barL.set_xdata([lo, lo])
        barR.set_xdata([hi, hi])
        _sync["axes"] = False

    ax.callbacks.connect("xlim_changed", _on_xlim_changed)

    # push slider views into nav history on mouse release
    tb = getattr(fig.canvas.manager, "toolbar", None)
    _last_view = [ax.get_xlim(), ax.get_ylim()] # closure box

    def _on_release(ev):
        if (ev.inaxes is slider_ax) and \
           (tb is not None) and \
           hasattr(tb, "push_current"):
            cur = [ax.get_xlim(), ax.get_ylim()]
            if cur != _last_view:
                tb.push_current() # record current view for back/forward
                _last_view[:] = cur # update snapshot

    fig.canvas.mpl_connect("button_release_event", _on_release)

    # format x values with ' for separator
    def _fmt(lo, hi):
        fmt = smarter_eng_formatter()
        # ' separator not supported natively
        # use , and replace at the end
        t = f"({int(lo):,d} - {int(hi):,d}) {fmt(hi+1-lo)}"
        return t.replace(",", "'")

    # update on change
    def _update_valtext(val):
        rs.valtext.set_text(_fmt(*val))

    # initial hex text
    _update_valtext(rs.val)

    rs.on_changed(_update_valtext)

    return rs

def attach_yrange_slider(ax, side="left", gap=0.02, w=0.035, no_ticks=False) ->\
RangeSlider:
    fig = ax.figure
    pos = ax.get_position()
    if side == "right":
        x0 = pos.x1 + gap
        keep_spine = "right"
        tick_left, tick_right = False, True
    else:
        x0 = pos.x0 - gap - w
        keep_spine = "left"
        tick_left, tick_right = True, False
    slider_ax = fig.add_axes([x0, pos.y0, w, pos.height])
    slider_ax._is_slider_ax = True

    y0, y1 = ax.get_ylim()
    inv = y0 > y1
    vmin, vmax = (y1, y0) if inv else (y0, y1)

    rs = RangeSlider(
        ax=slider_ax,
        label="",
        valmin=float(vmin),
        valmax=float(vmax),
        valinit=(float(vmin), float(vmax)),
        orientation="vertical",
        handle_style=dict(size=1, facecolor="w", edgecolor="w"),
    )

    if not no_ticks:
        # ticks copied from main Y (snapshot)
        slider_ax.set_axis_on()
        slider_ax.xaxis.set_visible(False)
        slider_ax.yaxis.set_visible(True)
        slider_ax.set_ylim(vmin, vmax)
        abs_ticks = ax.get_yticks()
        slider_ax.yaxis.set_major_locator(FixedLocator(abs_ticks))
        slider_ax.yaxis.set_major_formatter(ax.yaxis.get_major_formatter())

    # spines: keep only chosen side
    for s in ("top", "bottom", "left", "right"):
        slider_ax.spines[s].set_visible(False)
    slider_ax.spines[keep_spine].set_visible(True)
    slider_ax.tick_params(
        axis="y",
        left=tick_left, labelleft=tick_left,
        right=tick_right, labelright=tick_right,
        pad=1, length=3
    )

    # colors
    rs.poly.set_facecolor("white")
    rs.poly.set_alpha(0.8)
    rs.track.set_facecolor("black")
    rs.track.set_alpha(0.4)

    # bar handles (horizontal)
    barB = slider_ax.axhline(vmin, 0, 1, linewidth=6, color="k")
    barT = slider_ax.axhline(vmax, 0, 1, linewidth=6, color="k")

    # sync
    _sync = {"slider": False, "axes": False}

    def _on_slider(_):
        if _sync["axes"]: return
        _sync["slider"] = True
        lo, hi = rs.val
        if inv:
            ax.set_ylim(hi, lo)
        else:
            ax.set_ylim(lo, hi)
        barB.set_ydata([lo, lo])
        barT.set_ydata([hi, hi])
        fig.canvas.draw_idle()
        _sync["slider"] = False

    rs.on_changed(_on_slider)

    def _on_ylim_changed(event_ax):
        if _sync["slider"]: return
        _sync["axes"] = True
        ylo, yhi = event_ax.get_ylim()
        lo, hi = (min(ylo, yhi), max(ylo, yhi))
        rs.set_val((lo, hi))
        barB.set_ydata([lo, lo])
        barT.set_ydata([hi, hi])
        _sync["axes"] = False

    ax.callbacks.connect("ylim_changed", _on_ylim_changed)

    # push slider views into nav history on mouse release
    tb = getattr(fig.canvas.manager, "toolbar", None)
    _last_view = [ax.get_xlim(), ax.get_ylim()]

    def _on_release(ev):
        if (ev.inaxes is slider_ax) and \
           (tb is not None) and \
           hasattr(tb, "push_current"):
            cur = [ax.get_xlim(), ax.get_ylim()]
            if cur != _last_view:
                tb.push_current()
                _last_view[:] = cur

    fig.canvas.mpl_connect("button_release_event", _on_release)

    # format y values as hex - addresses
    def _fmt_hex(lo, hi):
        if no_ticks:
            return ""
        diff = hi-lo
        if diff >= 1024:
            diff /= 1024
            if diff < 10: digits = 2
            elif diff < 100: digits = 1
            else: digits=0
            sdiff = f"{diff:.{digits}f}KB"
        else:
            sdiff = f"{int(diff)}B"
        return f"(0x{int(lo):X} -\n0x{int(hi):X}) \n{sdiff}"

    # update on change
    def _update_valtext(val):
        rs.valtext.set_text(_fmt_hex(*val))

    # initial hex text
    _update_valtext(rs.val)

    rs.on_changed(_update_valtext)

    return rs

def link_xrange(ax1, rs1, ax2, rs2) -> None:
    """
    Keep two charts' X views in sync. Works across figures.
    Pass rs1/rs2 if there are sliders; use None if a chart has no slider.
    """
    shared = {"lock": False}

    def _copy_xlim(src_ax, dst_ax, dst_rs):
        if shared["lock"]:
            return
        shared["lock"] = True
        lo, hi = src_ax.get_xlim()
        dst_ax.set_xlim(lo, hi)
        if dst_rs is not None:
            dst_rs.set_val((lo, hi))
        shared["lock"] = False

    def _xl1(_):
        _copy_xlim(ax1, ax2, rs2)

    def _xl2(_):
        _copy_xlim(ax2, ax1, rs1)

    ax1.callbacks.connect("xlim_changed", _xl1)
    ax2.callbacks.connect("xlim_changed", _xl2)

def link_yrange(ax1, rs1, ax2, rs2) -> None:
    """
    Keep two charts' Y views in sync. Works across figures.
    Pass rs1/rs2 if there are sliders; use None if a chart has no slider.
    """
    shared = {"lock": False}

    def _copy_ylim(src_ax, dst_ax, dst_rs):
        if shared["lock"]:
            return
        shared["lock"] = True
        lo, hi = src_ax.get_ylim()
        dst_ax.set_ylim(lo, hi)
        if dst_rs is not None:
            dst_rs.set_val((min(lo, hi), max(lo, hi)))
        shared["lock"] = False

    def _yl1(_):
        _copy_ylim(ax1, ax2, rs2)

    def _yl2(_):
        _copy_ylim(ax2, ax1, rs1)

    ax1.callbacks.connect("ylim_changed", _yl1)
    ax2.callbacks.connect("ylim_changed", _yl2)

def on_xlim_changed_default(a, line, lw_off) -> None:
    def _on_xlim_changed(a):
        xmin, xmax = a.get_xlim()
        span = xmax - xmin
        # line for running acc
        lw = progressive_lw(span) * lw_off
        line.set_linewidth(lw)

    a.callbacks.connect("xlim_changed", _on_xlim_changed)

# single time series plots on existing ax
def plot_sp(ax, df) -> None:
    LW_OFF = .75
    lw = progressive_lw(df.index.size) * LW_OFF
    label = f"  SP max : {df['sp_real'].max()} B"
    line, = ax.step(
        df.smp, df.sp_real, where='post', lw=lw, color=CLR_TAB_BLUE)
    ax.grid(axis='both', linestyle='-', alpha=.6, which='major')
    ax.set_ylabel('Stack\nPointer')
    plot_dummy_line(ax, CLR_TAB_BLUE, 1.5, label)
    on_xlim_changed_default(ax, line, LW_OFF)

def plot_ipc(ax, df) -> None:
    LW_OFF = .65
    lw = progressive_lw(df.index.size) * LW_OFF

    ret_inst = df[df['inst'] != 0]['inst'].count()
    clks = df.smp.values[-1] - df.smp.values[0] + 1
    cpi = round(clks/ret_inst,3)
    ipc = round(1/cpi, 3)
    label = f" IPC: {ipc}\n(CPI: {cpi})"

    line, = ax.plot(df.smp, df.ipc_rolling, lw=lw, color=CLR_TAB_BLUE)
    plot_dummy_line(ax, CLR_TAB_BLUE, 1.5, label)
    H_OFFSET = .05
    ax.set_ylim(0-H_OFFSET, 1+H_OFFSET)
    on_xlim_changed_default(ax, line, LW_OFF)

def plot_stat(
    ax, x, y, unit, win_size, metric, agg='sum', clr=CLR_TAB_BLUE, yscale=1) \
    -> None:

    LW_OFF = .65
    lw = progressive_lw(len(y)) * LW_OFF
    if x.sum() == 0: # if data is all zeros, skip
        return ax

    fmt = EngFormatter(unit=unit, places=1)
    if agg == 'sum':
        x_agg = x.sum()
    elif agg == 'mean':
        x_agg = x.mean()
    elif agg != '':
        raise ValueError(f"Invalid aggregation '{agg}'. Use 'sum' or 'mean'.")

    xr = rolling_mean(x, win_size)
    xr /= yscale # to make y axis more readable, then manually label it outisde
    label = f"{metric}"
    if agg != '': # label it
        label += f": {fmt.format_eng(x_agg)}"

    line, = ax.plot(y, xr, lw=lw, color=clr)
    plot_dummy_line(ax, clr, 1.5, label)
    on_xlim_changed_default(ax, line, LW_OFF)

    ax.yaxis.set_major_locator(MaxNLocator(nbins=4, integer=True, prune=None))
    ax.yaxis.set_minor_locator(AutoMinorLocator(2))
    ax.grid(axis='y', linestyle='-', alpha=.6, which='major')
    ax.grid(axis='y', linestyle='--', alpha=0.4, which='minor')

def plot_hw_hm(ax, df, col, win_size) -> None:
    H_OFFSET = .3
    df[col] = df[col].replace(TRACE_NA, np.nan)
    hit = df[col].where(df[col] == 1, np.nan) + H_OFFSET
    miss = df[col].where(df[col] == 0, np.nan) - H_OFFSET
    alpha = progressive_alpha(df.index.size)
    bars_hit, = ax.plot(
        df.smp, hit,
        ls='None', marker="|", ms=8, alpha=alpha, c='g', label='hit')
    bars_miss, = ax.plot(
        df.smp, miss,
        ls='None', marker="|", ms=8, alpha=alpha, c='r', label='miss')

    metric = "ACC" if col == 'bp' else "HR"
    metric_mean = round(df[col].mean()*100,2)
    label = f"{metric}: {metric_mean}%"
    # drop NaNs to have continuous mean
    running_avg = rolling_mean(df[col].dropna(), win_size)
    # reindex and forward fill to account for NaNs in the source
    running_avg = running_avg.reindex(df.index, method='ffill')
    LW_OFF = .5
    lw = progressive_lw(df.index.size) * LW_OFF
    line, = ax.plot(df.smp, running_avg, lw=lw, color=CLR_TAB_BLUE)
    plot_dummy_line(ax, CLR_TAB_BLUE, 1.5, label)
    ax.set_ylim(-.8+H_OFFSET, 1.8-H_OFFSET)
    ax.set_yticks([0, .5, 1])
    ax.set_yticklabels(['0%', '', '100%'])

    def _on_xlim_changed(a):
        xmin, xmax = a.get_xlim()
        span = xmax - xmin
        # line for running acc
        lw = progressive_lw(span) * LW_OFF
        line.set_linewidth(lw)
        # alpha for hit/miss bars
        alpha = progressive_alpha(span)
        bars_hit.set_alpha(alpha)
        bars_miss.set_alpha(alpha)

    ax.callbacks.connect("xlim_changed", _on_xlim_changed)

# main drawing functions (entire figure)
def draw_freq(df, hl_inst_g, title, symbols, args, ctype) -> \
Tuple[plt.Figure, RangeSlider]:
    cols, ylabel = ctype_check(ctype)
    FS = (15,13)
    GS = {'top': .95, 'bottom': .06, 'left': .18, 'right': .66}
    fig, ax = plt.subplots(figsize=FS, gridspec_kw=GS)

    lw = progressive_lw(df.index.size)
    x_val, y_val = [], []
    for y, count, h in zip(df[cols[0]], df[cols[2]], df[cols[1]]):
        x_val.extend([count, count, np.nan])
        y_val.extend([y, y+h, np.nan])
    ax.plot(x_val, y_val, color=CLR_TAB_BLUE, lw=lw)
    ax.fill_betweenx(
        y_val, 0, x_val, where=~np.isnan(x_val), color=CLR_TAB_BLUE, alpha=.8)
    ax.set_xscale('log')

    if ctype == 'pc':
        # highlight specific instructions, if any
        hc = 0
        for hl_g in hl_inst_g:
            x_val_hl = []
            y_val_hl = []
            for inst in hl_g:
                df_hl = df[df['inst_mnm'] == inst]
                clr_hl_g = CLR_HL[hc]
                zipped = zip(df_hl[cols[0]], df_hl[cols[2]], df_hl[cols[1]])
                for y, count, h in zipped:
                    x_val_hl.extend([count, count, np.nan])
                    y_val_hl.extend([y, y+h, np.nan])
            ax.plot(x_val_hl, y_val_hl, color=clr_hl_g, lw=lw)
            ax.fill_betweenx(
                y_val_hl, 0, x_val_hl,
                where=~np.isnan(x_val_hl), color=clr_hl_g, alpha=.8
            )

            # add dummy scatter plot for the legend
            ax.scatter([], [], color=clr_hl_g, label=wrap_text(hl_g, 20))
            hc += 1

        if len(hl_inst_g) > 0:
            add_legend_for_hl_groups(ax, "trace_freq")

    ax = annotate_chart(df, symbols, ax, args, ctype)
    if args.add_cache_lines:
        ax = add_cache_line_spans(ax)

    # update axis
    #formatter = LogFormatter(base=10, labelOnlyBase=True)
    formatter = LogFormatterSciNotation(base=10)
    ax.xaxis.set_major_formatter(formatter)
    inc = find_loc_range(ax)
    ax.yaxis.set_major_locator(MultipleLocator(CACHE_LINE_BYTES*inc))
    ax.yaxis.set_major_formatter(FuncFormatter(int2hex))
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
    ax.set_title(get_title(TRACE_SOURCE, ylabel, title))
    ax.grid(axis='x', linestyle='-', alpha=1, which='major')
    ax.grid(axis='x', linestyle='--', alpha=0.6, which='minor')

    S_GAP = 0.08
    S_W = 0.03
    rsy = attach_yrange_slider(ax, gap=S_GAP, w=S_W)
    register_true_home(fig)

    return fig, rsy

def draw_timeline_exec(df, title, top_down=False) -> \
Tuple[plt.Figure, RangeSlider]:
    size = df['symbol'].unique().size
    FSY_DYN = size * 0.25 + 1 + 1 + (size < 8) # + sp + margin
    FSY = min(FSY_DYN, FS_EXEC_Y) # TODO: consider removing clamp?
    FS = (FS_EXEC_X, FSY)
    SCALE = (FS_EXEC_Y / FSY) # from og to current
    gse = GS_EXEC.copy()
    gse['bottom'] *= SCALE
    gse['top'] = 1 - (1 - gse['top']) * SCALE

    # set up figure
    fig, ax = plt.subplots(
        ncols=1, nrows=2, figsize=FS, sharex=True,
        height_ratios=[FSY-1, 1], gridspec_kw=gse)
    ax_tl, ax_sp = ax

    # add sp trace
    plot_sp(ax_sp, df)
    add_outside_legend(ax_sp, None)

    # add timeline
    smp = df['smp'].to_numpy()
    sym = df['symbol'].to_numpy()

    # detect runs
    start_idx = [0]
    for i in range(1, len(sym)):
        if sym[i] != sym[i-1]:
            start_idx.append(i)

    starts = np.array([smp[i] for i in start_idx])
    ends = np.r_[starts[1:], smp[-1]] # end of each run = start of next run
    widths = ends - starts
    run_syms = sym[start_idx]

    # assign lanes
    unique_syms = list(dict.fromkeys(run_syms)) # order-preserving uniq extract
    lane_index = {sym: i for i, sym in enumerate(unique_syms)}

    # cycle through default colors to get color map
    colors = plt.rcParams['axes.prop_cycle'].by_key()['color']
    color_map = {sym: colors[i % len(colors)]
                 for i, sym in enumerate(unique_syms)}

    # use vectorized mask once instead of per-run continue
    m_valid = widths > 0
    m_starts = starts[m_valid]
    m_ends = ends[m_valid]
    m_syms = run_syms[m_valid]
    # one Line2D per symbol, using NaN breaks
    for s in unique_syms:
        y = lane_index[s]
        m = (m_syms == s)
        if not np.any(m):
            continue

        seg_st = m_starts[m]
        seg_en = m_ends[m]
        # interleave as [start, end, nan, start2, end2, nan, ...] at constant y
        xs = np.empty(seg_st.size * 3, dtype=float)
        ys = np.empty_like(xs)
        xs[0::3] = seg_st
        xs[1::3] = seg_en
        xs[2::3] = np.nan
        ys[0::3] = y
        ys[1::3] = y
        ys[2::3] = np.nan
        ax_tl.plot(xs, ys, color_map[s], linewidth=13, solid_capstyle='butt')

    # update axis
    ax_tl.set_ylim(-0.5, len(unique_syms)-0.5)
    if top_down:
        ax_tl.invert_yaxis()
    ax_tl.margins(y=0.0, x=0.0)
    ax_tl.set_title(get_title(TRACE_SOURCE, TRACE_TYPE[0], title))
    ax_tl = add_spans(ax_tl)
    ax_r = ax_tl.twinx()
    ax_r.set_ylim(ax_tl.get_ylim())
    for a in ax_tl, ax_r:
        a.set_yticks(range(len(unique_syms)))
        a.set_yticklabels(unique_syms)

    #ax_tl.set_ylabel("Symbol")
    ax_tl.grid(axis='x', linestyle='-', alpha=.6, which='major')
    ax_top, xlabel = draw_exec_finish(ax_tl, df.smp[0], args.sample_begin_norm)
    ax_sp.set_xlabel(xlabel)

    S_GAP = 0.04
    S_H = 0.03 * SCALE
    S_W = (S_H / FS[0]) * FS[1]
    rsx = attach_xrange_slider(ax_sp, ax_top, gap=S_GAP*SCALE, h=S_H)
    _ = attach_yrange_slider(ax_tl, gap=S_GAP*1.5, w=S_W, no_ticks=True)
    register_true_home(fig)

    return fig, rsx

def draw_exec(df, hl_inst_g, title, symbols, args, ctype) -> \
Tuple[plt.Figure, RangeSlider, RangeSlider]:
    cols, ylabel = ctype_check(ctype)
    if not exec_df_within_limits(df.index.size, args.trace_limit):
        return None, None, None

    # set up figure
    FS = (FS_EXEC_X, FS_EXEC_Y)
    fig, ax_t = plt.subplots(figsize=FS, sharex=True, gridspec_kw=GS_EXEC)
    # add PC/DMEM trace
    LW_OFF_S = .75
    LW_OFF_HL = 1.
    lw = progressive_lw(df.index.size)
    step, = ax_t.step(
        df.smp, df[cols[0]], where='pre', lw=lw*LW_OFF_S, color=(0,.3,.6,.10))
    x_val, y_val = [], []
    for x, y, s in zip(df.smp, df[cols[0]], df[cols[1]]):
        x_val.extend([x, x, np.nan]) # 'np.nan' used to break the line
        y_val.extend([y, y+s, np.nan])
    line_mark, = ax_t.plot(x_val, y_val, color=CLR_TAB_BLUE, lw=lw)

    # add highlighted instructions
    hc = 0
    lines_hl = []
    if ctype == 'pc':
        trace_type = TRACE_TYPE[0]
        hl_off = .15
        for hl_g in hl_inst_g:
            x_val_hl = []
            y_val_hl = []
            for inst in hl_g:
                df_hl = df[df['inst_mnm'] == inst]
                for x, y, s in zip(df_hl.smp, df_hl[cols[0]], df_hl[cols[1]]):
                    x_val_hl.extend([x, x, np.nan])
                    y_val_hl.extend([y+hl_off, y+s-hl_off, np.nan])
            line, = ax_t.plot(
                x_val_hl, y_val_hl, color=CLR_HL[hc], lw=lw*LW_OFF_HL)
            lines_hl.append(line)

            # add dummy scatter plot for the legend
            ax_t.scatter(
                [], [], color=CLR_HL[hc], label=wrap_text(hl_g, 20))
            hc += 1

        if len(hl_inst_g) > 0:
            add_legend_for_hl_groups(ax_t, "trace_exec")

    elif ctype == 'dmem':
        trace_type = TRACE_TYPE[1]
        # dummy legend for load as it already exists
        ax_t.scatter([], [], color=CLR_TAB_BLUE, label='load')
        # and just highlight store in different color that load
        for hl_g, hl_c in zip(['store'], [CLR_HL[2]]):
            x_val_hl = []
            y_val_hl = []
            df_hl = df[df['dtyp'] == hl_g]
            for x, y, s in zip(df_hl.smp, df_hl['dmem'], df_hl['dsz']):
                x_val_hl.extend([x, x, np.nan])
                y_val_hl.extend([y, y+s, np.nan])
            line, = ax_t.plot(
                x_val_hl, y_val_hl, color=hl_c, lw=lw*LW_OFF_HL)
            lines_hl.append(line)

            # add dummy scatter plot for the legend
            ax_t.scatter([], [], color=hl_c, label=hl_g)
            add_legend_for_hl_groups(ax_t, "trace_exec")

    def _on_xlim_changed_lines(a):
        xmin, xmax = a.get_xlim()
        span = xmax - xmin
        # line for running acc
        lw = progressive_lw(span)
        step.set_linewidth(lw*LW_OFF_S)
        line_mark.set_linewidth(lw)
        for line_hl in lines_hl:
            line_hl.set_linewidth(lw*LW_OFF_HL)

    ax_t.callbacks.connect("xlim_changed", _on_xlim_changed_lines)

    ax_t = annotate_chart(df, symbols, ax_t, args, ctype)
    if args.add_cache_lines:
        ax_t = add_cache_line_spans(ax_t)

    # update axis
    inc = find_loc_range(ax_t)
    ax_t.yaxis.set_major_locator(MultipleLocator(CACHE_LINE_BYTES*inc))
    ax_t.yaxis.set_major_formatter(FuncFormatter(int2hex))

    def _on_ylim_changed_lines(a):
        inc = find_loc_range(a)
        a.yaxis.set_major_locator(MultipleLocator(CACHE_LINE_BYTES*inc))

    ax_t.callbacks.connect("ylim_changed", _on_ylim_changed_lines)

    ax_t.margins(y=0.03, x=0.0)
    ax_t.set_title(get_title(TRACE_SOURCE, trace_type, title))
    ax_t.set_ylabel(ylabel)
    ax_t.grid(axis='both', linestyle='-', alpha=.6, which='major')
    ax_top, xlabel = draw_exec_finish(ax_t, df.smp[0], args.sample_begin_norm)
    ax_t.set_xlabel(xlabel)

    S_GAP = 0.04
    S_H = 0.03
    S_W = (S_H / FS[0]) * FS[1]
    rsx = attach_xrange_slider(ax_t, ax_top, gap=S_GAP, h=S_H)
    rsy = attach_yrange_slider(ax_t, gap=S_GAP, w=S_W)
    register_true_home(fig)

    return fig, rsx, rsy

def draw_exec_finish(ax, xlim_low, norm) -> Tuple[plt.Axes, str]:
    # x
    max_n_locator = MaxNLocator(nbins=20, integer=True)
    ax.xaxis.set_major_locator(max_n_locator)
    ax.xaxis.set_major_formatter(EngFormatter(unit='', sep=''))
    ax.set_xlim(xlim_low-1, ax.get_xlim()[1]) # remove pad
    # add a second x-axis
    ax_top = ax.twiny()
    ax_top.set_xlim(ax.get_xlim())
    ax_top.xaxis.set_major_locator(max_n_locator)
    ax_top.xaxis.set_ticks_position('top')
    ax_top.xaxis.set_major_formatter(EngFormatter(unit='', sep=''))

    xlabel = 'Sample Count'
    xlabel += ' (normalized)' if norm else ''

    return ax_top, xlabel

def draw_stats_exec(df, title, args) -> Tuple[plt.Figure, RangeSlider]:
    if not exec_df_within_limits(df.index.size, args.trace_limit):
        return None, None

    nrows = 4 # ops, bp, ic, dc
    if args.clk:
        nrows += 1 # + ipc
        nrows += 4 # + ct_imem_core, ct_imem_mem, ct_dmem_core, ct_dmem_mem
        GS = GS_EXEC.copy()
        GS['hspace'] = .1
        FS = (FS_EXEC_X, FS_EXEC_Y)
        ops_n = "C" # ops per cycle
        S_GAP, S_H = 0.04, 0.03
    else:
        nrows += 1 # + branch density
        ops_n = "/inst"
        GS = {
            'top': .9, 'bottom': .18,
            'left': GS_EXEC['left'], 'right': GS_EXEC['right'],
            'hspace': .1
        }
        FS = (FS_EXEC_X, (FS_EXEC_Y/7)*nrows)
        S_GAP, S_H = 0.06, 0.04

    # set up figure
    fig, ax = plt.subplots(
        ncols=1, nrows=nrows, figsize=FS, sharex=True, gridspec_kw=GS)

    if args.clk:
        ax_ipc, ax_ops, ax_bp, ax_ic, ax_dc, \
            ax_ct_i2c, ax_ct_i2m, ax_ct_d2c, ax_ct_d2m = ax
        ax_btm = ax_ct_d2m
    else:
        ax_ops, ax_bd, ax_bp, ax_ic, ax_dc = ax
        ax_btm = ax_dc

    win_s = args.win_size_stats
    win_hw = args.win_size_hw
    y = df.smp
    plot_stat(ax_ops, df.ops.where(df.i_type==icfg.ALU, 0), y,
              'ops', win_s, 'ALU', clr=icfg.HL_COLORS_OPS[icfg.ALU])
    plot_stat(ax_ops, df.ops.where(df.i_type==icfg.MUL, 0), y,
              'ops', win_s, 'MUL', clr=icfg.HL_COLORS_OPS[icfg.MUL])
    #plot_stat(ax_ops, df.ops.where(df.i_type==icfg.DIV, 0), y,
    #          'ops', win_s, 'DIV', clr=icfg.HL_COLORS_OPS[icfg.DIV])
    plot_stat(ax_ops, df.ops.where(df.i_type==icfg.MEM, 0), y,
              'ops', win_s, 'MEM', clr=icfg.HL_COLORS_OPS[icfg.MEM])
    plot_stat(ax_ops, df.ops.where(df.i_type==icfg.SIMD, 0), y,
              'ops', win_s, 'SIMD', clr=icfg.HL_COLORS_OPS[icfg.SIMD])
    plot_stat(ax_ops, df.ops.where(df.i_type==icfg.UNPAK, 0), y,
              'ops', win_s, 'UNPAK', clr=icfg.HL_COLORS_OPS[icfg.UNPAK])

    plot_hw_hm(ax_bp, df, 'bp', win_hw)
    plot_hw_hm(ax_ic, df, 'ic', win_hw)
    plot_hw_hm(ax_dc, df, 'dc', win_hw)

    if args.clk:
        plot_ipc(ax_ipc, df)
        s = 1_000_000 # scale to MB/s
        w_clr = CLR_HL[2]
        agg_ct = 'mean'
        plot_stat(ax_ct_i2c, df.ct_imem_core, y, 'B/s', win_s,
                  'Read', agg=agg_ct, yscale=s)
        plot_stat(ax_ct_i2m, df.ct_imem_mem, y, 'B/s', win_s,
                  'Read', agg=agg_ct, yscale=s)
        plot_stat(ax_ct_d2c, df.ct_dmem_core_r, y, 'B/s', win_s,
                  'Read', agg=agg_ct, yscale=s)
        plot_stat(ax_ct_d2c, df.ct_dmem_core_w, y, 'B/s', win_s,
                  'Write', agg=agg_ct, clr=w_clr, yscale=s)
        plot_stat(ax_ct_d2m, df.ct_dmem_mem_r, y, 'B/s', win_s,
                  'Read', agg=agg_ct, yscale=s)
        plot_stat(ax_ct_d2m, df.ct_dmem_mem_w, y, 'B/s', win_s,
                  'Write', agg=agg_ct, clr=w_clr, yscale=s)
    else:
        plot_stat(ax_bd, df.br_dens.where(df.i_type==icfg.BRANCH, 0), y,
                  'ops', win_s, 'BRANCH', clr=icfg.HL_COLORS_OPS[icfg.BRANCH])
        plot_stat(ax_bd, df.br_dens.where(df.i_type==icfg.JUMP, 0), y,
                  'ops', win_s, 'JUMP', clr=icfg.HL_COLORS_OPS[icfg.JUMP])

    # update axis
    # x
    ax_btm.margins(x=0.0)
    max_n_locator = MaxNLocator(nbins=20, integer=True)
    ax_btm.xaxis.set_major_locator(max_n_locator)
    ax_btm.xaxis.set_major_formatter(EngFormatter(unit='', sep=''))
    ax_btm.set_xlim(df.smp[0]-1, ax_btm.get_xlim()[1]) # remove pad

    # add a second x-axis
    if args.clk:
        ax_top = ax_ipc.twiny()
    else:
        ax_top = ax_ops.twiny()
    ax_top.set_xlim(ax_btm.get_xlim())
    ax_top.xaxis.set_major_locator(max_n_locator)
    ax_top.xaxis.set_ticks_position('top')
    ax_top.xaxis.set_major_formatter(EngFormatter(unit='', sep=''))

    # title
    win_str = f" (W={win_s}, W_hw={win_hw})"
    ax_top.set_title(get_title(TRACE_SOURCE, "Stats & HW", title) + win_str)
    # axis labels
    ax_ops.set_ylabel(f'OP{ops_n}')
    ax_ic.set_ylabel('ICache')
    ax_dc.set_ylabel('DCache')
    ax_bp.set_ylabel('BP')
    if args.clk:
        ax_ipc.set_ylabel('IPC')
        ax_ct_i2c.set_ylabel("ICache to Core\n[MB/s]")
        ax_ct_i2m.set_ylabel("ICache to Mem\n[MB/s]")
        ax_ct_d2c.set_ylabel("DCache to Core\n[MB/s]")
        ax_ct_d2m.set_ylabel("DCache to Mem\n[MB/s]")
    else:
        ax_bd.set_ylabel('Flow Control\nDensity')

    ax[-1].set_xlabel(
        'Sample Count' + ' (normalized)' if args.sample_begin_norm else '')
    for a in ax:
        a.grid(axis='both', linestyle='-', alpha=.6, which='major')
        title = "Total OPS" if a == ax_ops else None
        add_outside_legend(a, title)

    rsx = attach_xrange_slider(ax_btm, ax_top, gap=S_GAP, h=S_H)
    register_true_home(fig)

    return fig, rsx

# data loading (json/bin)
def load_inst_prof(log, allow_internal=False) -> pd.DataFrame:
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

def load_bin_trace(bin_log, args) -> pd.DataFrame:
    dtype = np.dtype([
        ('smp', np.uint64),
        ('inst', np.uint32),
        ('pc', np.uint32),
        ('next_pc', np.uint32),
        ('dmem', np.uint32),
        ('sp', np.uint32),
        ('b_taken', np.uint8),
        ('isz', np.uint8),
        ('dsz', np.uint8),
        ('ic', np.uint8),
        ('dc', np.uint8),
        ('bp', np.uint8),
    ])

    if args.clk:
        dtype = np.dtype(dtype.descr + [
            ('ct_imem_core', np.uint8),
            ('ct_imem_mem', np.uint8),
            ('ct_dmem_core_r', np.uint8),
            ('ct_dmem_core_w', np.uint8),
            ('ct_dmem_mem_r', np.uint8),
            ('ct_dmem_mem_w', np.uint8),
        ])
        mem_stats = dtype.names[-6:]

    # recreate padding as done in the C++ trace entry struct
    el_max = np.max([dtype[item].itemsize for item in dtype.names])
    dsize = dtype.itemsize
    padding = ((((dsize + el_max) // el_max) * el_max) - dsize) % el_max
    pad_str = "pad_byte_"
    for p in range(padding):
        dtype = np.dtype(dtype.descr + [(pad_str+str(p), 'u1')])

    # load trace
    data = np.fromfile(bin_log, dtype=dtype)
    df = pd.DataFrame(data)
    df = df.drop(columns=[c for c in df.columns if pad_str in c])
    if args.sample_begin_norm:
        df.smp = df.smp - df.smp.head(1).values[0] # normalize smp to 0

    # enum class dmem_size_t {
    #     lb, lh, lw, ld,
    #     sb, sh, sw, sd,
    #     no_access
    # };
    # decode dsz to size in bytes and type (load/store) based on the above enum
    NA = 8 # no_access enum value
    df['dtyp'] = df['dsz'].apply(lambda x: 'store' if 7 >= x >= 4 else 'load')
    df['dsz'] = df['dsz'].apply(lambda x: x-4 if 7 >= x >= 4 else x)
    df['dsz'] = df['dsz'] + 1 # makes 1=byte, 2=half, 3=word, 4=dword
    df['dsz'] = df['dsz'].apply(lambda x: TRACE_NA if x == (NA + 1) else x)
    df['dsz'] = df['dsz'].apply(lambda x: 8 if x == 4 else x) # dw to 8B
    df['dsz'] = df['dsz'].apply(lambda x: 4 if x == 3 else x) # w to 4B
    df.loc[df['dsz'] == TRACE_NA, 'dtyp'] = ""

    # enum class hw_status_t { miss, hit, none };
    hw_status_t_none = 2
    df.ic = df.ic.replace(hw_status_t_none, TRACE_NA)
    df.dc = df.dc.replace(hw_status_t_none, TRACE_NA)
    df.bp = df.bp.replace(hw_status_t_none, TRACE_NA)
    # cpi in isa sim is just 1
    df['cpi'] = df[df['inst'] != 0]['smp'].diff().fillna(df['smp']).astype(int)
    if args.clk:
        df['ipc_inst'] = df['inst'].ne(0).astype(int)
        df['ipc_rolling'] = rolling_mean(df['ipc_inst'], args.win_size_stats)
        for m in mem_stats: # from bytes/cycle to bytes/sec
            df[m] = df[m].astype(np.uint16) * args.freq * 1_000_000

    df['sp_real'] = BASE_ADDR + MEM_SIZE - df['sp']
    df['sp_real'] = df['sp_real'].apply(
        lambda x: 0 if x >= BASE_ADDR + MEM_SIZE else x)

    df.inst = df.inst.replace(0, np.nan) # replace NOP/bubbles with NaN
    df.pc = df.pc.replace(0, np.nan) # same as for inst

    df_start = int(args.sample_begin) if args.sample_begin else df.smp.min()
    df_end = int(args.sample_end) if args.sample_end else df.smp.max()
    df = df.loc[df['smp'].between(df_start, df_end)]

    return df

# main run functions
def run_inst_profile(log, hl_inst_g, title, args) -> \
Tuple[pd.DataFrame, plt.Figure]:

    df = load_inst_prof(log)
    fig = draw_inst_profile(df, hl_inst_g, title, args)

    return df, fig

def run_bin_trace(bin_log, hl_inst_g, title, args) -> \
Tuple[pd.DataFrame, plt.Figure]:
    df_t = load_bin_trace(bin_log, args)
    df_ret = None
    figs_ret = {}

    df_ret, df_t_ops, fig_tl, rsx_tl, fig_pc, fig2_pc, rsx2_pc = \
        run_bin_trace_pc(df_t, hl_inst_g, title, args)
    figs_ret['tl'] = fig_tl
    figs_ret['pc'] = fig_pc
    figs_ret['pc_exec'] = fig2_pc

    fig_dmem, fig2_dmem, rsx2_dmem = \
        run_bin_trace_dmem(df_t, title, args)
    figs_ret['dmem'] = fig_dmem
    figs_ret['dmem_exec'] = fig2_dmem

    fig_hw, rsx_hw = None, None
    if args.stats_trace:
        if not args.dasm:
            raise ValueError("Error: --stats_trace requires --dasm to map ops")
        fig_hw, rsx_hw = draw_stats_exec(df_t_ops, title, args)
        figs_ret['hw_exec'] = fig_hw

    figs_exec = [fig_tl, fig2_pc, fig2_dmem, fig_hw]
    rsx_exec = [rsx_tl, rsx2_pc, rsx2_dmem, rsx_hw]

    # link x ranges of all exec figs that exist
    first_ax = None
    first_rs = None
    for fige, rse in zip(figs_exec, rsx_exec):
        if fige is not None:
            if first_ax is None:
                first_ax = fige.get_axes()[0]
                first_rs = rse
            else:
                link_xrange(first_ax, first_rs, fige.get_axes()[0], rse)

    return df_ret, figs_ret

def run_bin_trace_pc(df_t, hl_inst_g, title, args) -> \
Tuple[pd.DataFrame, pd.DataFrame,
        plt.Figure, RangeSlider,
        plt.Figure, plt.Figure, RangeSlider]:

    df_f = df_t.groupby('pc').agg(
        isz=('isz', 'first'), # get only the first value
        count=('cpi', 'sum') # count them all (size of the group)
    ).reset_index()
    df_f = df_f.sort_values(by='pc', ascending=True)
    df_f['pc'] = df_f['pc'].astype(int)
    df_f['pc_hex'] = df_f['pc'].apply(lambda x: f'{x:08x}')

    #if args.pc_begin:
    #    df_t = df_t.loc[df_t.pc > hex2int(args.pc_begin)]
    #if args.pc_end:
    #    df_t = df_t.loc[df_t.pc < hex2int(args.pc_end)]

    symbols = {}
    m_hl_groups = []
    if args.dasm:
        m_hl_groups = hl_inst_g
        symbols, df_map = backannotate_dasm(args, df_f, "text")

        # merge df_map into df_f by keeping only records in the df_f
        df_f = pd.merge(df_f, df_map, how='left', left_on='pc', right_on='pc')
        df_f['count'] = df_f['count'].astype(int) # after merge, count is float
        df_t = pd.merge(df_t, df_map, how='left', left_on='pc', right_on='pc')

        df_t['i_type'] = df_t['inst_mnm'].map(icfg.INST_TO_TYPE_DICT)
        df_t['i_mem_type'] = df_t['inst_mnm'].map(icfg.INST_TO_MEM_TYPE_DICT)
        df_t['ops'] = df_t['inst_mnm'].map(icfg.INST_TO_OPS_DICT)
        df_t['ops'] = df_t['ops'].fillna(0).astype(int) # fill NaN ops with 0
        if not args.clk:
            df_t['br_dens'] = df_t['inst_mnm'].map(icfg.INST_TO_BD_DICT)

        # build an inclusive IntervalIndex and parallel labels array
        starts, ends, labels = [], [], []
        for name, m in symbols.items():
            starts.append(int(m['addr_start']))
            ends.append(int(m['addr_end']))
            labels.append(name)

        if args.clk:
            FILL_SYM = {"addr": BASE_ADDR-4, "name": "special"}
            starts.append(FILL_SYM['addr'])
            ends.append(FILL_SYM['addr'])
            labels.append(FILL_SYM['name'])

        ii = pd.IntervalIndex.from_arrays(starts, ends, closed='both')
        if args.clk:
            df_t['pc'] = df_t['pc'].replace(np.nan, FILL_SYM['addr'])

        # index each pc into the interval set
        idx = ii.get_indexer(df_t['pc'].astype(int))

        # map matches to labels; unmatched -> NA
        out = np.full(len(df_t), pd.NA, dtype=object)
        mask = idx != -1
        labels_arr = np.asarray(labels, dtype=object)
        out[mask] = labels_arr[idx[mask]]
        df_t['symbol'] = out

        if args.clk:
            # special back to NaN
            df_t['symbol'] = df_t['symbol'].replace(FILL_SYM['name'], pd.NA)
            df_t['pc'] = df_t['pc'].replace(FILL_SYM['addr'], np.nan)
            # forward fill to have continuous symbols in timeline
            # and backfill to cover leading NaNs
            df_t['symbol'] = df_t['symbol'].ffill().bfill()

    if args.save_converted_trace:
        df_s = df_t.copy()
        # revert NaNs to 0 for saving as hex
        df_s.inst = df_s.inst.replace(np.nan, 0).astype(int)
        df_s.pc = df_s.pc.replace(np.nan, 0).astype(int)
        if args.dasm:
            # drop 'dtyp' to avoid duplication
            # as 'i_type' & 'i_mem_type' columns are added with dasm parsing
            df_s = df_s.drop(columns=['dtyp'])
        # some columns to hex
        for c in ["inst"]:
            df_s[c] = df_s[c].apply(lambda x: f'{x:08X}')
        for c in ["pc", "next_pc", "dmem", "sp"]:
            digits = int(math.ceil(np.log2(BASE_ADDR)/4))
            df_s[c] = df_s[c].apply(lambda x: f'{x:0{digits}X}')
        df_s.to_csv(args.trace.replace('.bin', '.bin.csv'), index=False)

    fig_tl, fig, rsy, rsx_tl, fig2, rsx2, rsy2 = [None] * 7
    if args.symbols_only or \
        not (args.pc_freq or args.pc_trace or args.timeline):
        # early exit, no plotting for PC
        return df_f, df_t, fig_tl, rsx_tl, fig, fig2, rsx2

    if args.timeline:
        fig_tl, rsx_tl = draw_timeline_exec(df_t, title)
    if args.pc_freq:
        fig, rsy = draw_freq(df_f, m_hl_groups, title, symbols, args, 'pc')
    if args.pc_trace:
        fig2, rsx2, rsy2 = draw_exec(
            df_t, m_hl_groups, title, symbols, args, 'pc')

    # link y ranges if both figures exist
    if rsy != None and rsy2 != None:
        link_yrange(fig.get_axes()[0], rsy, fig2.get_axes()[0], rsy2)

    return df_f, df_t, fig_tl, rsx_tl, fig, fig2, rsx2

def run_bin_trace_dmem(df_t, title, args) -> \
Tuple[plt.Figure, plt.Figure, RangeSlider]:

    fig, rsy, fig2, rsx2, rsy2 = [None] * 5
    if not (args.symbols_only or args.dmem_freq or args.dmem_trace):
        return fig, fig2, rsx2

    symbols = {}
    if args.dasm:
        symbols, _ = backannotate_dasm(args, None, "data")

    if args.symbols_only or not (args.dmem_freq or args.dmem_trace):
        return fig, fig2, rsx2

    df_t['dmem'] = df_t['dmem'].replace(0, np.nan) # gaps in dmem acces/chart

    #if args.dmem_begin:
    #    df_t = df_t.loc[df_t.dmem > hex2int(args.dmem_begin)]
    #if args.dmem_end:
    #    df_t = df_t.loc[df_t.dmem < hex2int(args.dmem_end)]

    def expand_byte_accesses(df_t: pd.DataFrame) -> pd.DataFrame:
        # Work only on rows with dsz > 0
        df = df_t.loc[df_t['dsz'] > 0, ['dmem', 'dsz', 'dtyp']]

        # repeat indices
        rep_idx = np.repeat(df.index.to_numpy(), df['dsz'].to_numpy())
        out = df.loc[rep_idx, ['dmem', 'dtyp']].copy()
        out['dsz'] = 1

        # offset within each original row
        out['dmem'] = out['dmem'].to_numpy() + \
            out.groupby(level=0).cumcount().to_numpy()

        out.reset_index(drop=True, inplace=True)
        return out

    # NOTE: expand_byte_accesses takes awfully long time for large traces
    if args.dmem_freq:
        # to bytes as ISA is byte addressable, combined accesses for freq plot
        df_exp = expand_byte_accesses(df_t)
        df = df_exp.groupby('dmem').agg(
            dsz=('dsz', 'first'), # get only the first value
            count=('dmem', 'size') # count them all (size of the group)
        ).reset_index()
        df = df.sort_values(by='dmem', ascending=True)
        df['dmem'] = df['dmem'].astype(int)
        df['dmem_hex'] = df['dmem'].apply(lambda x: f'{x:08x}')

        # FIXME: why was this ever needed? some corner case?
        #if df.iloc[0]['dmem'] == 0:
        #    df = df.drop(0)

        fig, rsy = draw_freq(df, [], title, symbols, args, ctype='dmem')

    if args.dmem_trace:
        fig2, rsx2, rsy2 = draw_exec(df_t, [], title, symbols, args, 'dmem')

    # link y ranges if both figures exist
    if rsy != None and rsy2 != None:
        link_yrange(fig.get_axes()[0], rsy, fig2.get_axes()[0], rsy2)

    return fig, fig2, rsx2

# args
def parse_args() -> argparse.Namespace:
    TRACE_LIMIT = 1_000_000 # should be able to do more than this easily, but make user aware they have a lot of samples
    parser = argparse.ArgumentParser(description="Analysis of memory access logs and traces")
    # either
    parser.add_argument('-i', '--inst_profile', type=str, help="Path to instruction profiling output 'inst_profile.json'")
    # or
    parser.add_argument('-t', '--trace', type=str, help="Path to binary execution trace 'trace_clk.bin'")

    # instruction count log only options
    parser.add_argument('--exclude', type=inst_exists, nargs='+', help="Exclude specific instructions. Instruction profile only option")
    parser.add_argument('--exclude_type', type=inst_type_exists, nargs='+', help=f"Exclude specific instruction types. Available types are: {', '.join(icfg.INST_T.keys())}. Instruction profile only option")
    parser.add_argument('--top', type=int, help="Number of N most common instructions to display. Default is all. Instruction profile only option")
    parser.add_argument('--allow_zero', action='store_true', default=False, help="Allow instructions with zero count to be displayed. Instruction profile only option")

    # trace only options
    parser.add_argument('--dasm', type=str, help="Path to disassembly 'dasm' file to backannotate the trace. This file and the new annotated file is saved in the same directory as the input trace with *.prof.<original ext> suffix. Trace only option")

    parser.add_argument('--sample_begin', type=int, help="Show only samples after this sample. Applied before any other filtering options. Trace only option")
    parser.add_argument('--sample_end', type=int, help="Show only samples before this sample. Applied before any other filtering options. Trace only option")
    parser.add_argument('--clk', action='store_true', help="Trace is from the RTL simulation with real clock. Trace only option") # TODO: can it be autodetected?
    parser.add_argument('--freq', default=FREQ_DEFAULT, help="Core clock frequency in [MHz]. Trace only option")
    parser.add_argument('--sample_begin_norm', action='store_true', help="Normalize trace start to 0th sample. Trace only option")

    parser.add_argument('--timeline', action='store_true', help="Plot top of stack trace as timeline. Requires --dasm. Trace only option")
    parser.add_argument('--pc_freq', action='store_true', help="Plot PC frequency. Each instruction as a separate entry. Trace only option")
    parser.add_argument('--pc_trace', action='store_true', help="Plot PC time trace. Each executed instruction is plotted using its respective size (2/4 B). Trace only option")
    parser.add_argument('--no_pc_limit', action='store_true', help="Don't limit the PC range to the execution trace range. Only updates plot view. Applied after --sample_begin/end. Useful when logging is done with HINT instruction. By default, the PC range is limited to the execution trace range. Trace only option")
    parser.add_argument('--pc_begin', type=str, help="Show only PCs after this PC (hex). Applied after --no_pc_limit. Trace only option")
    parser.add_argument('--pc_end', type=str, help="Show only PCs before this PC (hex). Applied after --no_pc_limit. Trace only option")

    parser.add_argument('--dmem_freq', action='store_true', help="Plot DMEM frequency. Each byte is plotted as a separate entry. Trace only option")
    parser.add_argument('--dmem_trace', action='store_true', help="Plot DMEM time trace. Each accessed memory location is plotted using its respective size (1/2/4 B). Trace only option")
    parser.add_argument('--no_dmem_limit', action='store_true', help="Don't limit the DMEM range to the execution trace range. Only updates plot view. Applied after --sample_begin/end. Same as --no_pc_limit but for DMEM. Trace only option")
    parser.add_argument('--dmem_begin', type=str, help="Show only DMEM addresses after this address (hex). Applied after --no_dmem_limit. Trace only option")
    parser.add_argument('--dmem_end', type=str, help="Show only DMEM addresses before this address (hex). Applied after --no_dmem_limit. Trace only option")

    parser.add_argument('--stats_trace', action='store_true', help="Plot stats and hardware (model) counters time trace. Trace only option. Eequires trace from either ISA sim with HW models or RTL sim")
    parser.add_argument('--win_size_stats', type=int, default=32, help="Number of samples to use for rolling average window for stats plots (IPC, OPC, etc.). Trace only option")
    parser.add_argument('--win_size_hw', type=int, default=16, help="Number of samples to use for rolling average window for hardware (model) plots. Trace only option")

    parser.add_argument('--add_cache_lines', action='store_true', default=False, help="Add alternate coloring for cache lines (both Icache and Dcache). Useful to inspect data locality. Trace only option")
    parser.add_argument('--symbols_only', action='store_true', help="Only backannotate and display the symbols found in the 'dasm' file. Requires --dasm. Doesn't display figures and ignores all save options except --save_csv. Trace only option")
    parser.add_argument('--save_symbols', action='store_true', help="Save the symbols found in the 'dasm' file as a JSON file. Requires --dasm. Trace only option")
    parser.add_argument('--print_symbols', action='store_true', help="Print symbols from dasm to the stdout. Requires --dasm. Trace only option")
    parser.add_argument('--trace_limit', type=int, default=TRACE_LIMIT, help=f"Limit the number of address entries to display in the time series chart. Default is {TRACE_LIMIT}. Trace only option")
    parser.add_argument('--save_converted_trace', action='store_true', help="Save the converted binary trace as a CSV file. Trace only option")

    # common options
    parser.add_argument('--highlight', '--hl', type=str, nargs='+', help="Highlight specific instructions. Multiple instructions can be provided as a single string separated by whitespace (multiple groups) or separated by commas (multiple instructions in a group). E.g.: 'add,addi sub' colors 'add' and 'addi' the same and 'sub' a different color. Defaults to predefined groups if not provided. Use 'off' to disable highlighting")
    # TODO: add highlighting for function calls/PC ?
    parser.add_argument('-b', '--browser', action='store_true', help="Open plots in the web browser instead of a pop-up window")
    parser.add_argument('--host', action='store_true', help="Host server for all machines on LAN instead of local-only. Only applicable if used with --browser")
    parser.add_argument('-s', '--silent', action='store_true', help="Don't display chart(s)")
    parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
    parser.add_argument('--save_svg', action='store_true', help="Save charts as SVG")
    parser.add_argument('--save_csv', action='store_true', help="Save summary data used for charts formatted as CSV")

    return parser.parse_args()

# main
def run_main(args) -> None:
    run_inst = args.inst_profile is not None
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

    at_least_one = \
        args.timeline or args.pc_freq or args.pc_trace or \
        args.dmem_freq or args.dmem_trace or \
        args.stats_trace
    data_run = args.save_converted_trace or args.save_csv
    if run_trace and not at_least_one and not data_run:
        raise ValueError("At least one trace-based plot needs to be specified "
                         "or trace needs to be saved for trace run")

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
        if args.sample_begin >= args.sample_end:
            raise ValueError("--sample_begin must be less than --sample_end")

    # check PC filtering
    if (args.pc_begin and args.pc_end):
        if int(args.pc_begin, 16) >= int(args.pc_end, 16):
            raise ValueError("--pc_begin must be less than --pc_end")

    # check DMEM filtering
    if (args.dmem_begin and args.dmem_end):
        if int(args.dmem_begin, 16) >= int(args.dmem_end, 16):
            raise ValueError("--dmem_begin must be less than --dmem_end")

    args_log = args.trace if run_trace else args.inst_profile
    if not os.path.exists(args_log):
        raise FileNotFoundError(f"File {args_log} not found")

    hl_inst_g = []
    if (args.highlight == ["off"]):
        pass
    elif (args.highlight != None):
        if len(args.highlight) > len(CLR_HL):
            raise ValueError(f"Too many instructions to highlight " + \
                             f"max is {len(CLR_HL)}, " + \
                             f"got {len(args.highlight)}")
        else:
            if len(args.highlight) == 1: # multiple arguments but 1 element
                hl_inst_g = args.highlight[0].split()
            else: # already split on whitespace (somehow?)
                hl_inst_g = args.highlight
            hl_inst_g = [ah.split(",") for ah in hl_inst_g]
    else:
        hl_inst_g = icfg.HL_DEFAULT

    fig_arr = []
    ext = os.path.splitext(args_log)[1]
    title = get_test_title(args_log)
    log_path = os.path.realpath(args_log)

    if args.browser:
        matplotlib.use("WebAgg")
        if args.host:
            import subprocess
            host_ip = subprocess.check_output(
                ["hostname", "-I"]).decode().split()[0]
            matplotlib.rcParams['webagg.address'] = host_ip
            matplotlib.rcParams['webagg.open_in_browser'] = False

    if run_trace:
        df, figs_dict = run_bin_trace(args_log, hl_inst_g, title, args)
        for name, fig in figs_dict.items():
            fig_arr.append([log_path.replace(ext, f"_{name}{ext}"), fig])
    else:
        df, fig = run_inst_profile(args_log, hl_inst_g, title, args)
        fig_arr.append([log_path, fig])
        figs_dict = {"inst": fig}

    if args.save_csv:
        df.to_csv(args_log.replace(ext, "_summary.csv"), index=False)

    if args.symbols_only and run_trace:
        return

    if args.save_png: # each chart is saved as a separate PNG file
        for name, fig in (fig_arr):
            fig.savefig(name.replace(" ", "_").replace(ext, ".png"))

    if args.save_svg: # each chart is saved as a separate SVG file
        for name, fig in (fig_arr):
            fig.savefig(name.replace(" ", "_").replace(ext, ".svg"))

    if not args.silent:
        if args.browser:
            # for browser-based, just show the plots
            plt.show()
        else:
            # if not, figure out the run env, add helper to close all windows
            plt.show(block=False)
            if not is_notebook():
                input("Press Enter to close all plots...")
    plt.close('all')

if __name__ == "__main__":
    args = parse_args()
    TRACE_SOURCE = "RTL" if args.clk else "ISA sim"
    run_main(args)
