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
from matplotlib.ticker import (FixedLocator, FuncFormatter, LogFormatter,
                               LogFormatterSciNotation, MaxNLocator,
                               MultipleLocator)
from matplotlib.widgets import RangeSlider
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

generic_blue = "#649ac9"

# memory instructions breakdown store vs load
inst_mem_bd = {
    MEM_L : ["Load", colors["blue_light1"]],
    MEM_S : ["Store", colors["blue_light2"]],
}

def hex2int(addr) -> int:
    return int(addr,16) # - int(BASE_ADDR)

def int2hex(val, pos) -> str:
    return f"0x{int(val):04X}"

def get_count(parts, df) -> Tuple[int, int]:
    pc = hex2int(parts[0].strip())
    count_series  = df.loc[df["pc"] == pc, "count"]
    count = count_series.squeeze() if not count_series.empty else 0
    return count, pc

def to_k(val, pos) -> str:
    if val == 0:
        return "0"
    if val/1000 == val//1000:
        return f"{val//1000:.0f}k"
    return f"{val/1000:.3f}k"

def to_v(val, pos) -> str:
    return f"{val}"

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

def wrap_text(arr:List[str], max_len:int) -> str:
    label = ', '.join(arr)
    wrapped = '\n'.join(textwrap.wrap(label, max_len))
    return wrapped

def add_legend_for_hl_groups(ax, chart_type:str) -> None:
    title = "Highlighted\nInstructions\n"
    if chart_type == "log":
        ax.legend(loc='lower right', title=title, framealpha=0.5)

    # put legend in the top right corner, outside the plot
    elif chart_type == "trace_freq":
        ax.legend(title=title, framealpha=0.5, loc='upper right',
                  bbox_to_anchor=(1.52, 1.03), borderaxespad=0.)
    elif chart_type == "trace_exec":
        ax.legend(title=title, framealpha=0.5, loc='upper right',
                  bbox_to_anchor=(1.2, 1.03), borderaxespad=0.)

    else:
        raise ValueError(f"Invalid chart type '{chart_type}'")

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

def draw_inst_log(df, hl_inst_g, title, args) -> plt.Figure:
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
    for hl_g in hl_inst_g:
        for i, r in enumerate(box[0]):
            if df.iloc[i]['name'] in hl_g:
                r.set_color(hl_colors[hc])
        ax[0].barh(0, 0, color=hl_colors[hc], label=', '.join(hl_g))
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

def run_inst_log(log, hl_inst_g, title, args) -> \
Tuple[pd.DataFrame, plt.Figure]:

    df = json_prof_to_df(log)
    fig = draw_inst_log(df, hl_inst_g, title, args)

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
                    addr_start = hex2int(parts[0].strip())
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

    sym_log = []
    for k,v in symbols.items():
        v['symbol_text'] = f"{k}"
        if section == "text":
            v['symbol_text'] += f" ({v['exec_count']})"

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

    df_out = pd.DataFrame(pc_inst_map_arr, columns=['pc', 'inst_mnm', 'inst'])
    return symbols, df_out

def add_cache_line_spans(ax) -> None:
    top = (int(ax.get_ylim()[1]) & ~0x3F) + CACHE_LINE_BYTES
    bottom = int(ax.get_ylim()[0]) & ~0x3F
    for i in range(bottom, top, CACHE_LINE_BYTES):
        if (i//CACHE_LINE_BYTES) % 2 != 0:
            # color every other line in gray
            continue
        ax.axhspan(i, i+CACHE_LINE_BYTES, color='k', alpha=0.05, zorder=0)

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
        ax.set_ylim(top=(int(df[ctype].max()) & ~0x3F) + CACHE_LINE_BYTES)
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

# sliders
def register_true_home(fig):
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

def attach_xrange_slider(ax, ax_top, gap=0.03, h=0.035):
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
    rs.poly.set_facecolor("white"); rs.poly.set_alpha(0.8)
    rs.track.set_facecolor("black"); rs.track.set_alpha(0.4)

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
        barL.set_xdata([lo, lo]); barR.set_xdata([hi, hi])
        fig.canvas.draw_idle()
        _sync["slider"] = False

    rs.on_changed(_on_slider)

    def _on_xlim_changed(event_ax):
        if _sync["slider"]: return
        _sync["axes"] = True
        lo, hi = event_ax.get_xlim()
        rs.set_val((lo, hi))
        barL.set_xdata([lo, lo]); barR.set_xdata([hi, hi])
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
        # ' separator not supported natively
        # use , and replace at the end
        t = f"({int(lo):,d} - {int(hi):,d}) {(hi-lo)/1000:.3f}k"
        return t.replace(",", "'")

    # update on change
    def _update_valtext(val):
        rs.valtext.set_text(_fmt(*val))

    # initial hex text
    _update_valtext(rs.val)

    rs.on_changed(_update_valtext)

    return rs

def attach_yrange_slider(ax, side="left", gap=0.02, w=0.035):
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
    rs.poly.set_facecolor("white"); rs.poly.set_alpha(0.8)
    rs.track.set_facecolor("black"); rs.track.set_alpha(0.4)

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
        barB.set_ydata([lo, lo]); barT.set_ydata([hi, hi])
        fig.canvas.draw_idle()
        _sync["slider"] = False

    rs.on_changed(_on_slider)

    def _on_ylim_changed(event_ax):
        if _sync["slider"]: return
        _sync["axes"] = True
        ylo, yhi = event_ax.get_ylim()
        lo, hi = (min(ylo, yhi), max(ylo, yhi))
        rs.set_val((lo, hi))
        barB.set_ydata([lo, lo]); barT.set_ydata([hi, hi])
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
        diff = hi-lo
        if diff >= 1024:
            diff /= 1024
            if diff < 10: digits = 2
            elif diff < 100: digits = 1
            else: digits=0
            sdiff = f"{diff:.{digits}f}KB"
        else:
            sdiff = f"{int(diff)}B"
        return f"(0x{int(lo):X} - 0x{int(hi):X})\n{sdiff}"

    # update on change
    def _update_valtext(val):
        rs.valtext.set_text(_fmt_hex(*val))

    # initial hex text
    _update_valtext(rs.val)

    rs.on_changed(_update_valtext)

    return rs

def link_xrange(ax1, rs1, ax2, rs2):
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

def link_yrange(ax1, rs1, ax2, rs2):
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

# drawing
def draw_freq(df, hl_inst_g, title, symbols, args, ctype) -> plt.Figure:
    cols, ylabel = ctype_check(ctype)
    FS = (14,12)
    fig, ax = plt.subplots(
        figsize=FS,
        gridspec_kw={'top': .95, 'bottom': .1, 'left': .18, 'right': .7}
    )

    lw = progressive_lw(df.index.size)
    x_val, y_val = [], []
    for y, count, h in zip(df[cols[0]], df[cols[2]], df[cols[1]]):
        x_val.extend([count, count, np.nan])
        y_val.extend([y, y+h, np.nan])
    ax.plot(x_val, y_val, color=generic_blue, lw=lw)
    ax.fill_betweenx(
        y_val, 0, x_val, where=~np.isnan(x_val), color=generic_blue, alpha=.8)
    ax.set_xscale('log')

    if ctype == 'pc':
        # highlight specific instructions, if any
        hc = 0
        for hl_g in hl_inst_g:
            x_val_hl = []
            y_val_hl = []
            for inst in hl_g:
                df_hl = df[df['inst_mnm'] == inst]
                clr = hl_colors[hc]
                zipped = zip(df_hl[cols[0]], df_hl[cols[2]], df_hl[cols[1]])
                for y, count, h in zipped:
                    x_val_hl.extend([count, count, np.nan])
                    y_val_hl.extend([y, y+h, np.nan])
            ax.plot(x_val_hl, y_val_hl, color=clr, lw=lw)
            ax.fill_betweenx(
                y_val_hl, 0, x_val_hl,
                where=~np.isnan(x_val_hl), color=clr, alpha=.8
            )

            # add dummy scatter plot for the legend
            ax.scatter([], [], color=clr, label=wrap_text(hl_g, 16))
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
    ax.set_title(f"{ylabel} frequency profile for {title}")

    ax.grid(axis='x', linestyle='-', alpha=1, which='major')
    ax.grid(axis='x', linestyle='--', alpha=0.6, which='minor')

    S_GAP = 0.08
    S_W = 0.03
    rsy = attach_yrange_slider(ax, gap=S_GAP, w=S_W)
    register_true_home(fig)

    return fig, rsy

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
    right_bound = 1_000_000
    left_out = 1.5
    right_out = .2
    return map_value(x, left_bound, right_bound, left_out, right_out)

def plot_sp(ax, df):
    LW_OFF = .75
    lw = progressive_lw(df.index.size) * LW_OFF
    line, =  ax.step(
        df.smp, df.sp_real, where='post', lw=lw, color=(0,.3,.6,.7))

    def _on_xlim_changed(a):
        xmin, xmax = a.get_xlim()
        span = xmax - xmin
        # line for running acc
        lw = progressive_lw(span) * LW_OFF
        line.set_linewidth(lw)

    ax.callbacks.connect("xlim_changed", _on_xlim_changed)

def plot_hw_hm(ax, df, col, hw_win_size):
    H_OFFSET = .3
    hit = df[col].where(df[col] == 1, np.nan) + H_OFFSET
    miss = df[col].where(df[col] == 0, np.nan) - H_OFFSET
    alpha = progressive_alpha(df.index.size)
    bars_hit, = ax.plot(
        df.smp, hit,
        ls='None', marker="|", ms=8, alpha=alpha, c='g', label='hit')
    bars_miss, = ax.plot(
        df.smp, miss,
        ls='None', marker="|", ms=8, alpha=alpha, c='r', label='miss')

    # drop NaNs to have continuous mean
    running_avg = df[col] \
                  .dropna() \
                  .rolling(window=hw_win_size, min_periods=1) \
                  .mean()
    # reindex and forward fill to account for NaNs in the source
    running_avg = running_avg.reindex(df.index, method='ffill')
    LW_OFF = .5
    lw = progressive_lw(df.index.size) * LW_OFF
    line, = ax.plot(
        df.smp, running_avg, lw=lw, color=(0,.3,.6,.7), label='avg')
    ax.set_ylim(-.8+H_OFFSET, 1.8-H_OFFSET)
    ax.set_yticks([0, .5, 1])
    ax.set_yticklabels(['0%', '', '100%'])
    #ax.legend(loc='upper right', ncol=3)

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

    metric = "ACC" if col == 'bp' else "HR"
    metric_mean = round(df[col].mean()*100,2)
    ax.text(1, 1,
            f"  {metric}: {metric_mean}%", color='k',
            fontsize=9, ha='left', va='center',
            bbox=dict(facecolor='w', alpha=0.6, lw=0, pad=1),
            transform=ax.get_yaxis_transform())

def draw_exec(df, hl_inst_g, title, symbols, args, ctype) -> plt.Figure:
    cols, ylabel = ctype_check(ctype)
    if len(df) > args.trace_limit:
        print(f"Warning: too many PC entries to display in the time series " + \
              f"chart ({len(df)}). Limit is {args.trace_limit} " + \
              f"entries. Either increase the limit or filter the data.")
        return None

    FS = (25,12)
    # set up figure
    fig, ax = plt.subplots(
        ncols=1, nrows=5, figsize=FS, sharex=True,
        height_ratios=[10, 1.2, 1.2, 1.2, 1],
        gridspec_kw={'top': .95, 'bottom': .1, 'left': .09, 'right': .84})
    ax_t, ax_bp, ax_ic, ax_dc, ax_sp = ax

    # add grid
    ax_t.grid(axis='x', linestyle='-', alpha=.6, which='major')
    for a in ax:
        a.grid(axis='both', linestyle='-', alpha=.6, which='major')

    # add sp and hw model traces
    plot_sp(ax_sp, df)
    plot_hw_hm(ax_bp, df, 'bp', args.hw_win_size)
    plot_hw_hm(ax_ic, df, 'ic', args.hw_win_size)
    plot_hw_hm(ax_dc, df, 'dc', args.hw_win_size)

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
    line_mark, = ax_t.plot(x_val, y_val, color=generic_blue, lw=lw)

    # add highlighted instructions
    hc = 0
    lines_hl = []
    if ctype == 'pc':
        trace_type = "Execution"
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
                x_val_hl, y_val_hl, color=hl_colors[hc], lw=lw*LW_OFF_HL)
            lines_hl.append(line)

            # add dummy scatter plot for the legend
            ax_t.scatter(
                [], [], color=hl_colors[hc], label=wrap_text(hl_g, 16))
            hc += 1

        if len(hl_inst_g) > 0:
            add_legend_for_hl_groups(ax_t, "trace_exec")

    elif ctype == 'dmem':
        trace_type = "Data"
        # dummy legend for load as it already exists
        ax_t.scatter([], [], color=generic_blue, label='load')
        # and just highlight store in different color that load
        for hl_g, hl_c in zip(['store'], [hl_colors[2]]):
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

    ax_sp.text(1, df['sp_real'].max(),
               f"  SP max : {df['sp_real'].max()} bytes", color='k',
               fontsize=9, ha='left', va='center',
               bbox=dict(facecolor='w', alpha=0.6, lw=0, pad=1),
               transform=ax_sp.get_yaxis_transform())

    # update axis
    # y
    inc = find_loc_range(ax_t)
    ax_t.yaxis.set_major_locator(MultipleLocator(CACHE_LINE_BYTES*inc))
    ax_t.yaxis.set_major_formatter(FuncFormatter(int2hex))
    ax_t.margins(y=0.03, x=0.0)
    # x
    max_n_locator = MaxNLocator(nbins=20, integer=True)
    ax_t.xaxis.set_major_locator(max_n_locator)
    ax_t.xaxis.set_major_formatter(FuncFormatter(to_k))
    # add a second x-axis
    ax_top = ax_t.twiny()
    ax_top.set_xlim(ax_t.get_xlim())
    ax_top.xaxis.set_major_locator(max_n_locator)
    ax_top.xaxis.set_ticks_position('top')
    ax_top.xaxis.set_major_formatter(FuncFormatter(to_k))

    def _on_xlim_changed_locator(a):
        # on less than THR samples, fix at 1k
        THR = 10_000
        STEP = 1000
        xmin, xmax = a.get_xlim()
        samples = xmax - xmin
        want_fixed = (samples <= THR)
        have_fixed = isinstance(a.xaxis.get_major_locator(), FixedLocator)
        if want_fixed == have_fixed:
            return  # already in the right mode

        start = np.floor(xmin / STEP) * STEP
        stop  = np.ceil(xmax / STEP) * STEP
        fixed_ticks = np.arange(start, stop + 0.5 * STEP, STEP)
        if samples > THR:
            #a.xaxis.set_major_formatter(FuncFormatter(to_k))
            a.xaxis.set_major_locator(max_n_locator) # the og from above
        else:
            #a.xaxis.set_major_formatter(FuncFormatter(to_v))
            a.xaxis.set_major_locator(FixedLocator(fixed_ticks))
    ax_t.callbacks.connect('xlim_changed', _on_xlim_changed_locator)
    ax_top.callbacks.connect('xlim_changed', _on_xlim_changed_locator)

    # label
    ax_t.set_title(f"{trace_type} profile for {title}")
    ax_t.set_ylabel(ylabel)
    ax_sp.set_ylabel('Stack Pointer')
    ax_ic.set_ylabel('ICache')
    ax_dc.set_ylabel('DCache')
    ax_bp.set_ylabel('Branch\nPredictor')

    ax[-1].set_xlabel('Sample Count')

    S_GAP = 0.04
    S_H = 0.03
    S_W = (S_H / FS[0]) * FS[1]
    rsx = attach_xrange_slider(ax_sp, ax_top, gap=S_GAP, h=S_H)
    rsy = attach_yrange_slider(ax_t, gap=S_GAP, w=S_W)
    register_true_home(fig)

    return fig, rsx, rsy

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
        dfs.to_csv(bin_log.replace('.bin', '.bin.csv'), index=False)

    df_start = int(args.sample_begin) if args.sample_begin else 0
    df_end = int(args.sample_end) if args.sample_end else df.smp.max()
    df = df.loc[df['smp'].between(df_start,df_end)]

    return df

def run_bin_trace(bin_log, hl_inst_g, title, args) -> \
Tuple[pd.DataFrame, plt.Figure, plt.Figure]:
    df = load_bin_trace(bin_log, args)
    df_out = None
    dict_out = {}
    df_out, fig_pc, fig2_pc, rsx2_pc = \
        run_bin_trace_pc(df, hl_inst_g, title, args)
    dict_out['pc'] = fig_pc
    dict_out['pc_exec'] = fig2_pc
    _, fig_dmem, fig2_dmem, rsx2_dmem = \
        run_bin_trace_dmem(df, title, args)
    dict_out['dmem'] = fig_dmem
    dict_out['dmem_exec'] = fig2_dmem

    # link x ranges if both figures exist
    if rsx2_pc != None and rsx2_dmem != None:
        link_xrange(
            fig2_pc.get_axes()[0], rsx2_pc,
            fig2_dmem.get_axes()[0], rsx2_dmem
        )

    return df_out, dict_out

def run_bin_trace_pc(df_og, hl_inst_g, title, args) -> \
Tuple[pd.DataFrame, plt.Figure, plt.Figure]:

    df = df_og.groupby('pc').agg(
        isz=('isz', 'first'), # get only the first value
        count=('smp_taken', 'sum') # count them all (size of the group)
    ).reset_index()
    df = df.sort_values(by='pc', ascending=True)
    df['pc'] = df['pc'].astype(int)
    df['pc_hex'] = df['pc'].apply(lambda x: f'{x:08x}')

    #if args.pc_begin:
    #    df_og = df_og.loc[df_og.pc > hex2int(args.pc_begin)]
    #if args.pc_end:
    #    df_og = df_og.loc[df_og.pc < hex2int(args.pc_end)]

    symbols = {}
    m_hl_groups = []
    if args.dasm:
        m_hl_groups = hl_inst_g
        symbols, df_map = backannotate_dasm(args, df, "text")
        # merge df_map into df by keeping only records in the df
        df = pd.merge(df, df_map, how='left', left_on='pc', right_on='pc')
        df_og = pd.merge(df_og, df_map, how='left', left_on='pc', right_on='pc')

    fig, rsy, fig2, rsx2, rsy2 = [None] * 5
    if args.symbols_only or not (args.pc_freq or args.pc_trace):
        # early exit, no plotting for PC
        return df, fig, fig2, rsx2

    if args.pc_freq:
        fig, rsy = draw_freq(df, m_hl_groups, title, symbols, args, 'pc')
    if args.pc_trace:
        fig2, rsx2, rsy2 = draw_exec(df_og, m_hl_groups, title, symbols, args, 'pc')

    # link y ranges if both figures exist
    if rsy != None and rsy2 != None:
        link_yrange(
            fig.get_axes()[0], rsy,
            fig2.get_axes()[0], rsy2
        )

    return df, fig, fig2, rsx2

def expand_byte_accesses(df_og: pd.DataFrame) -> pd.DataFrame:
    # Work only on rows with dsz > 0
    df = df_og.loc[df_og['dsz'] > 0, ['dmem', 'dsz', 'dtyp']]

    # repeat indices
    rep_idx = np.repeat(df.index.to_numpy(), df['dsz'].to_numpy())
    out = df.loc[rep_idx, ['dmem', 'dtyp']].copy()
    out['dsz'] = 1

    # offset within each original row
    out['dmem'] = out['dmem'].to_numpy() + \
        out.groupby(level=0).cumcount().to_numpy()

    out.reset_index(drop=True, inplace=True)
    return out

def run_bin_trace_dmem(df_og, title, args) -> \
Tuple[pd.DataFrame, plt.Figure, plt.Figure]:

    df_og['dtyp'] = df_og['dsz'].apply(lambda x: 'store' if x >= 8 else 'load')
    df_og['dsz'] = df_og['dsz'].apply(lambda x: x-8 if x >= 8 else x)
    df_og['dmem'] = df_og['dmem'].replace(0, np.nan) # gaps in dmem acces/chart

    #if args.dmem_begin:
    #    df_og = df_og.loc[df_og.dmem > hex2int(args.dmem_begin)]
    #if args.dmem_end:
    #    df_og = df_og.loc[df_og.dmem < hex2int(args.dmem_end)]

    # to bytes as ISA is byte addressable, accesses are combined for freq plot
    df_exp = expand_byte_accesses(df_og)
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

    fig, rsy, fig2, rsx2, rsy2 = [None] * 5
    if args.symbols_only or not (args.dmem_freq or args.dmem_trace):
        return df, fig, fig2, rsx2

    if args.dmem_freq:
        fig, rsy = draw_freq(df, [], title, symbols, args, ctype='dmem')
    if args.dmem_trace:
        fig2, rsx2, rsy2 = draw_exec(df_og, [], title, symbols, args, 'dmem')

    # link y ranges if both figures exist
    if rsy != None and rsy2 != None:
        link_yrange(
            fig.get_axes()[0], rsy,
            fig2.get_axes()[0], rsy2
        )

    return df, fig, fig2, rsx2

def parse_args() -> argparse.Namespace:
    TRACE_LIMIT = 1_000_000 # should be able to do more than this easily, but make user aware they have a lot of samples
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

    parser.add_argument('--hw_win_size', type=int, default=16, help="Number of samples to use for rolling average window for branch predictor accuracy and caches hit rate. Trace only option")
    parser.add_argument('--add_cache_lines', action='store_true', default=False, help="Add alternate coloring for cache lines (both Icache and Dcache). Useful to inspect data locality. Trace only option")
    parser.add_argument('--symbols_only', action='store_true', help="Only backannotate and display the symbols found in the 'dasm' file. Requires --dasm. Doesn't display figures and ignores all save options except --save_csv. Trace only option")
    parser.add_argument('--save_symbols', action='store_true', help="Save the symbols found in the 'dasm' file as a JSON file. Requires --dasm. Trace only option")
    parser.add_argument('--print_symbols', action='store_true', help="Print symbols from dasm to the stdout. Requires --dasm. Trace only option")
    parser.add_argument('--trace_limit', type=int, default=TRACE_LIMIT, help=f"Limit the number of address entries to display in the time series chart. Default is {TRACE_LIMIT}. Trace only option")
    parser.add_argument('--save_converted_trace', action='store_true', help="Save the converted binary trace as a CSV file. Trace only option")

    # common options
    parser.add_argument('--highlight', '--hl', type=str, nargs='+', help="Highlight specific instructions. Multiple instructions can be provided as a single string separated by whitespace (multiple groups) or separated by commas (multiple instructions in a group). E.g.: 'add,addi sub' colors 'add' and 'addi' the same and 'sub' a different color.")
    # TODO: add highlighting for function calls/PC ?
    parser.add_argument('-b', '--browser', action='store_true', help="Open plots in the web browser instead of a pop-up window")
    parser.add_argument('--host', action='store_true', help="Host server for all machines on LAN instead of local-only. Only applicable if used with --browser")
    parser.add_argument('-s', '--silent', action='store_true', help="Don't display chart(s)")
    parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
    parser.add_argument('--save_svg', action='store_true', help="Save charts as SVG")
    parser.add_argument('--save_csv', action='store_true', help="Save summary data used for charts formatted as CSV")

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

    at_least_one = \
        args.pc_freq or args.pc_trace or args.dmem_freq or args.dmem_trace
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

    hl_inst_g = []
    if not (args.highlight == None):
        if len(args.highlight) > len(hl_colors):
            raise ValueError(f"Too many instructions to highlight " + \
                             f"max is {len(hl_colors)}, " + \
                             f"got {len(args.highlight)}")
        else:
            if len(args.highlight) == 1: # multiple arguments but 1 element
                hl_inst_g = args.highlight[0].split()
            else: # already split on whitespace (somehow?)
                hl_inst_g = args.highlight
            hl_inst_g = [ah.split(",") for ah in hl_inst_g]

    fig_arr = []
    ext = os.path.splitext(args_log)[1]
    title = os.path.basename(os.path.dirname(args_log)).replace("out_", "")
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
        df, fig = run_inst_log(args_log, hl_inst_g, title, args)
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
    run_main(args)
