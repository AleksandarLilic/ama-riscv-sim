import os
import json
import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from typing import Dict, Any, Tuple, List
from matplotlib.ticker import MultipleLocator, FuncFormatter, LogFormatter, LogFormatterSciNotation, MaxNLocator

#SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
ARITH = "ARITH"
MEM = "MEM"
BRANCH = "BRANCH"
JUMP = "JUMP"
CSR = "CSR"
ENV = "ENV"
NOP = "NOP"
FENCE = "FENCE"
MEM_S = "MEM_S"
MEM_L = "MEM_L"

INST_BYTES = 4
CACHE_LINE_INSTS = 64//INST_BYTES
BASE_ADDR = 0x80000000
MEM_SIZE = 0x40000
# offset is requred to move bars/dots up by half a unit
# in order not to be centered around the PC but start from it
# compressed ISA whill change this, when supported
PC_OFFSET = 0.5

inst_t_mem = {
    MEM_S: ["sb", "sh", "sw"],
    MEM_L: ["lb", "lh", "lw", "lbu", "lhu"],
}

inst_t = {
    ARITH: [
        "add", "sub", "sll", "srl", "sra", "slt", "sltu", "xor", "or", "and",
        "addi", "slli", "srli", "srai", "slti", "sltiu", "xori", "ori", "andi",
        "lui", "auipc"
    ],
    MEM: inst_t_mem[MEM_S] + inst_t_mem[MEM_L],
    BRANCH: ["beq", "bne", "blt", "bge", "bltu", "bgeu"],
    JUMP: ["jalr", "jal"],
    CSR: ["csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"],
    ENV: ["ecall", "ebreak"],
    NOP: ["nop"],
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

highlight_colors = [
    #"#33C1B1", # keppel
    "#3ECCBB", # turquoise
    #"#e9c46a", # yellow
    #"#ECCE83", # jasmine
    "#EED595", # peach yellow
    "#f4a261", # orange
    "#e76f51", # red
    "#2a9d8f", # teal
]

# memory instructions breakdown store vs load
inst_mem_bd = {
    MEM_L : ["Load", colors["blue_light1"]],
    MEM_S : ["Store", colors["blue_light2"]],
}

def get_base_int_pc(pc) -> int:
    return int(pc,16) - int(BASE_ADDR)

def get_count(parts, df) -> Tuple[int, int]:
    pc = get_base_int_pc(parts[0].strip())
    count_series  = df.loc[df["pc_real"] == pc, "count"]
    count = count_series.squeeze() if not count_series.empty else 0
    return count, pc

def num_to_hex(val, pos) -> str:
    return f"0x{int(val*INST_BYTES):04X}"

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

def parse_args() -> argparse.Namespace:
    TIME_SERIES_LIMIT = 50000
    parser = argparse.ArgumentParser(description="Analyze instruction count and Trace log")
    # either
    parser.add_argument('-i', '--inst_log', type=str, help="JSON instruction count log with profiling data")
    # or
    parser.add_argument('-t', '--trace', type=str, help="Binary execution trace")

    # instruction count log only options
    parser.add_argument('--exclude', type=inst_exists, nargs='+', help="Exclude specific instructions. Instruction count log only option")
    parser.add_argument('--exclude_type', type=inst_type_exists, nargs='+', help=f"Exclude specific instruction types. Available types are: {', '.join(inst_t.keys())}. Instruction count log only option")
    parser.add_argument('--top', type=int, help="Number of N most common instructions to display. Default is all. Instruction count log only option")
    parser.add_argument('--allow_zero', action='store_true', default=False, help="Allow instructions with zero count to be displayed. Instruction count log only option")

    # trace only options
    parser.add_argument('--dasm', type=str, help="Path to disassembly 'dasm' file to backannotate the Trace. New file is generated at the same path with *.prof.<original ext> suffix. Trace only option")
    parser.add_argument('--pc_begin', type=str, help="Show only PCs after this PC. Input is a hex string. E.g. '0x80000094'. Trace only option")
    parser.add_argument('--pc_end', type=str, help="Show only PCs before this PC. Input is a hex string. E.g. '0x800000ec'. Trace only option")
    parser.add_argument('--symbols_only', action='store_true', help="Only backannotate and display the symbols found in the 'dasm' file. Requires --dasm. Doesn't display figures and ignores all save options except --save_csv. Trace only option")
    parser.add_argument('--save_symbols', action='store_true', help="Save the symbols found in the 'dasm' file as a JSON file. Requires --dasm. Trace only option")
    parser.add_argument('--pc_time_series_limit', type=int, default=TIME_SERIES_LIMIT, help=F"Limit the number of PC entries to display in the time series chart. Default is {TIME_SERIES_LIMIT}. Trace only option")

    # common options
    parser.add_argument('--highlight', '--hl', type=str, nargs='+', help="Highlight specific instructions. Multiple instructions can be provided as a single string separated by whitespace (multiple groups) or separated by commas (multiple instructions in a group). E.g.: 'add,addi sub' colors 'add' and 'addi' the same and 'sub' a different color.")
    # TODO: add highlighting for function calls/PC ?
    parser.add_argument('-s', '--silent', action='store_true', help="Don't display chart(s) in pop-up window")
    parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
    parser.add_argument('--save_svg', action='store_true', help="Save charts as SVG")
    parser.add_argument('--save_csv', action='store_true', help="Save source data formatted as CSV")

    return parser.parse_args()

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
    suptitle_str += f"\nInstructions profiled: {inst_profiled}"
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
                r.set_color(highlight_colors[hc])
        ax[0].barh(0, 0, color=highlight_colors[hc], label=', '.join(hl_g))
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

    # handle display
    if not args.silent:
        plt.show()

    plt.close()
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

def backannotate_dasm(args, df) -> \
Tuple[Dict[str, Dict[str, int]], pd.DataFrame]:

    symbols = {}
    pc_inst_map_arr = []
    dasm_ext = os.path.splitext(args.dasm)[1]
    with open(args.dasm, 'r') as infile, \
    open(args.dasm.replace(dasm_ext, '.prof' + dasm_ext), 'w') as outfile:
        current_sym = None
        append = False
        NUM_DIGITS = len(str(df['count'].max())) + 1

        for line in infile:
            if line.startswith('Disassembly of section .text'):
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
                    pc_start = get_base_int_pc(parts[0].strip())
                    symbol_name = parts[1][1:-1] # remove <> from symbol name
                    if current_sym:
                        symbols[current_sym]['pc_end_real'] = prev_pc
                    current_sym = symbol_name
                    symbols[current_sym] = {
                        'pc_start_real': pc_start,
                        "acc_count": 0
                    }
                    prev_pc = pc_start

                if len(parts) == 2 and parts[1].startswith('\t'):
                    # detected instruction
                    count, prev_pc = get_count(parts, df)
                    outfile.write("{:{}} {}".format(count, NUM_DIGITS, line))
                    symbols[current_sym]['acc_count'] += count

                    im = line.split('\t')
                    im = [x.strip() for x in im]
                    pc_inst_map_arr.append(
                        [get_base_int_pc(im[0].replace(':', ''))//4, # pc
                         im[2], # instruction mnemonic
                         ' '.join(im[2:]) # full instruction
                         ])

                else: # not an instruction
                    outfile.write(line)

            else: # not .text section
                outfile.write(line)

        # write the last symbol
        symbols[current_sym]['pc_end_real'] = prev_pc

    filter_str = []
    if args.pc_begin:
        filter_str.append(f"PC >= {args.pc_begin}")
        pc_begin = get_base_int_pc(args.pc_begin)
        symbols = {k: v for k, v in symbols.items()
                   if v['pc_start_real'] >= pc_begin}

    if args.pc_end:
        filter_str.append(f"PC <= {args.pc_end}")
        pc_end = get_base_int_pc(args.pc_end)
        symbols = {k: v for k, v in symbols.items()
                   if v['pc_end_real'] <= pc_end}

    sym_log = []
    for k,v in symbols.items():
        v['func_text'] = f"{k} ({v['acc_count']})"
        sym_log.append(f"{num_to_hex(v['pc_start_real']//4, None)} - " + \
                       f"{num_to_hex(v['pc_end_real']//4, None)}: " + \
                       f"{v['func_text']}")

    print(f"Symbols found in {args.dasm}:")
    if filter_str:
        print(f"Filtered by: {' and '.join(filter_str)}")
    for sym in sym_log[::-1]:
        print(sym)

    if args.save_symbols:
        # convert to python types first
        symbols_py = {}
        for k,v in symbols.items():
            for k2,v2 in v.items():
                if isinstance(v2, np.int64):
                    v[k2] = int(v2)
            symbols_py[k] = dict(v)

        with open(args.dasm.replace(dasm_ext, '_symbols.json'), 'w') as sym_file:
            json.dump(symbols_py, sym_file, indent=4)

    df_out = pd.DataFrame(pc_inst_map_arr, columns=['pc', 'inst_mnm', 'inst'])
    return symbols, df_out

def annotate_pc_chart(df, symbols, ax, symbol_pos=None) -> plt.Axes:
    if symbol_pos is None:
        symbol_pos = ax.get_xlim()[1]

    pc_start, pc_end = 0, 0
    # add lines for symbols, if any
    for k,v in symbols.items():
        pc_start = v['pc_start_real']//INST_BYTES
        pc_end = v['pc_end_real']//INST_BYTES
        ax.axhline(y=pc_start, color='k', linestyle='-', alpha=0.5)
        ax.text(symbol_pos, pc_start,
                f" ^ {v['func_text']}", color='k',
                fontsize=9, ha='left', va='center',
                bbox=dict(facecolor='w', alpha=0.6, linewidth=0, pad=1))

    # add line for the last symbol, if any
    if symbols:
        # ends after last PC entry
        ax.axhline(y=pc_end+1, color='k', linestyle='-', alpha=0.5)

    if args.pc_begin:
        ax.set_ylim(bottom=get_base_int_pc(args.pc_begin)//4)
    if args.pc_end:
        ax.set_ylim(top=get_base_int_pc(args.pc_end)//4)

    # add cache line coloring
    for i in range(df.pc.min(), max(df.pc.max(), pc_end), CACHE_LINE_INSTS):
        color = 'k' if (i//CACHE_LINE_INSTS) % 2 == 0 else 'w'
        ax.axhspan(i, i+CACHE_LINE_INSTS, color=color, alpha=0.08, zorder=0)

    return ax

def draw_pc_freq(df, hl_groups, title, symbols, args) -> plt.Figure:
    # TODO: should probably limit the number of PC entries to display or
    # auto enlarge the figure size (but up to a point, then limit anyways)
    fig, ax = plt.subplots(figsize=(12,15), constrained_layout=True)
    box = ax.barh(df['pc']+PC_OFFSET, df['count'], height=.9, alpha=0.8)
    #       , edgecolor=(0, 0, 0, 0.1))
    ax.set_xscale('log')

    # highlight specific instructions, if any
    hc = 0
    for hl_g in hl_groups:
        for i, r in enumerate(box):
            if df.iloc[i]['inst_mnm'] in hl_g:
                # has to be removed as set_color doesn't respect original height
                r.remove()
                # add a new bar with the highlight color
                ax.barh(df.iloc[i]['pc']+PC_OFFSET, df.iloc[i]['count'],
                        height=.9, color=highlight_colors[hc], alpha=0.8)
        # add a dummy bar for the legend
        ax.barh(0, 0, color=highlight_colors[hc], label=', '.join(hl_g))
        hc += 1

    if len(hl_groups) > 0:
        add_legend_for_hl_groups(ax, "trace")

    # update axis
    #formatter = LogFormatter(base=10, labelOnlyBase=True)
    formatter = LogFormatterSciNotation(base=10)
    ax.xaxis.set_major_formatter(formatter)
    ax.yaxis.set_major_locator(MultipleLocator(CACHE_LINE_INSTS))
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
    ax.set_ylabel("Program Counter")
    ax.set_title(f"PC frequency profile for {title}")

    ax.grid(axis='x', linestyle='-', alpha=1, which='major')
    ax.grid(axis='x', linestyle='--', alpha=0.6, which='minor')

    # annotate the chart
    ax = annotate_pc_chart(df, symbols, ax)

    # handle display
    if not args.silent:
        plt.show()

    plt.close()
    return fig

def draw_pc_exec(df, hl_groups, title, symbols, args) -> plt.Figure:
    fig, [ax_pc, ax_sp] = plt.subplots(ncols=1, nrows=2, figsize=(24,12),
                                       sharex=True, height_ratios=[9, 1],
                                       constrained_layout=True)
    # plot the PC trace and don't interpolate linearly between points
    # but step from one to the next
    ax_pc.step(df.index, df['pc']+PC_OFFSET, where='post', linewidth=2,
            color=(0,.3,.6,0.2), marker='.', markersize=5,
            markerfacecolor='#649ac9', markeredgecolor='#649ac9')
    ax_pc.grid(axis='x', linestyle='-', alpha=1, which='major')

    # scatter plot points from df where inst matches inst_mnm
    hc = 0
    for hl_g in hl_groups:
        for inst in hl_g:
            df_hl = df[df['inst_mnm'] == inst]
            ax_pc.scatter(df_hl.index, df_hl['pc']+PC_OFFSET,
                       color=highlight_colors[hc], s=10, zorder=10)
        # add a dummy scatter plot for the legend
        ax_pc.scatter([], [], color=highlight_colors[hc], label=', '.join(hl_g))
        hc += 1

    if len(hl_groups) > 0:
        add_legend_for_hl_groups(ax_pc, "trace")

    # update axis
    current_nbins = len(ax_pc.get_xticks())
    locator = MaxNLocator(nbins=current_nbins*4, integer=True)
    ax_pc.xaxis.set_major_locator(locator)
    ax_pc.xaxis.set_major_formatter(FuncFormatter(to_k))
    ax_pc.yaxis.set_major_locator(MultipleLocator(CACHE_LINE_INSTS))
    ax_pc.yaxis.set_major_formatter(FuncFormatter(num_to_hex))
    ax_pc.margins(y=0.03, x=0.01)

    # add a second x-axis
    ax_top = ax_pc.twiny()
    ax_top.set_xlim(ax_pc.get_xlim())
    ax_top.xaxis.set_major_locator(locator)
    ax_top.xaxis.set_ticks_position('top')
    ax_top.xaxis.set_major_formatter(FuncFormatter(to_k))

    # add SP trace
    ax_sp.step(df.index, df['sp_real'], where='post', linewidth=1,
               color=(0,.3,.6,1))
    ax_sp.grid(axis='both', linestyle='-', alpha=1, which='major')

    # label
    ax_sp.set_xlabel('Instruction Count')
    ax_pc.set_ylabel('Program Counter')
    ax_sp.set_ylabel('Stack Pointer')
    ax_pc.set_title(f"Execution profile for {title}")

    # annotate both
    ax_pc = annotate_pc_chart(df, symbols, ax_pc)
    ax_sp.text(ax_sp.get_xlim()[1], df['sp_real'].max(),
               f"  SP max : {df['sp_real'].max()} bytes", color='k',
               fontsize=9, ha='left', va='center',
               bbox=dict(facecolor='w', alpha=0.6, linewidth=0, pad=1))

    # handle display
    if not args.silent:
        plt.show()

    plt.close()
    return fig

def run_pc_trace(bin_log, hl_groups, title, args) -> \
Tuple[pd.DataFrame, plt.Figure, plt.Figure]:

    h = ['pc', 'sp']
    dtype = np.dtype([
        (h[0], np.uint32),
        (h[1], np.uint32),
    ])
    data = np.fromfile(bin_log, dtype=dtype)
    df_og = pd.DataFrame(data, columns=h)
    df_og['sp_real'] = BASE_ADDR + MEM_SIZE - df_og['sp']
    df_og['sp_real'] = df_og['sp_real'].apply(
        lambda x: 0 if x == BASE_ADDR + MEM_SIZE else x)

    df = df_og.groupby('pc').size().reset_index(name='count')
    df = df.sort_values(by='pc', ascending=True)
    df['pc'] = df['pc'].astype(int)
    df['pc_hex'] = df['pc'].apply(lambda x: f'{x:08x}')
    df['pc_real'] = df['pc'] * INST_BYTES
    df['pc_real_hex'] = df['pc_real'].apply(lambda x: f'{x:08x}')

    # TODO: consider option to reverse the PC log so it grows down from the top
    # like .dump does

    symbols = {}
    m_hl_groups = []
    if args.dasm:
        m_hl_groups = hl_groups
        symbols, df_map = backannotate_dasm(args, df)
        # merge df_map into df by keeping only records in the df
        df = pd.merge(df, df_map, how='left', left_on='pc', right_on='pc')
        df_og = pd.merge(df_og, df_map, how='left', left_on='pc', right_on='pc')

    if args.symbols_only:
        return df, None, None

    fig = draw_pc_freq(df, m_hl_groups, title, symbols, args)

    if len(df_og) > args.pc_time_series_limit:
        print(f"Warning: too many PC entries to display in the time series " + \
              f"chart ({len(df_og)}). Limit is {args.pc_time_series_limit} " + \
              f"entries. Either increase the limit or filter the data.")
        fig2 = None
    else:
        fig2 = draw_pc_exec(df_og, m_hl_groups, title, symbols, args)

    return df, fig, fig2

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

    if (args.pc_begin or args.pc_end):
        if not args.trace:
            raise ValueError(
                "--pc_begin and --pc_end require single execution trace")
        if not args.dasm:
            raise ValueError("--pc_begin and --pc_end require --dasm")
        if int(args.pc_begin, 16) >= int(args.pc_end, 16):
            raise ValueError("--pc_begin must be less than --pc_end")

    if run_trace:
        args_log = args.trace
        profiler_str = "_trace.bin"
    else:
        args_log = args.inst_log
        profiler_str = "_inst_profiler.json"

    if not os.path.exists(args_log):
        raise FileNotFoundError(f"File {args_log} not found")

    hl_groups = []
    if not (args.highlight == None):
        if len(args.highlight) > len(highlight_colors):
            raise ValueError(f"Too many instructions to highlight " + \
                            f"max is {len(highlight_colors)}, " + \
                            f"got {len(args.highlight)}")
        else:
            if len(args.highlight) == 1: # multiple arguments but 1 element
                hl_groups = args.highlight[0].split()
            else: # already split on whitespace (somehow?)
                hl_groups = args.highlight
            hl_groups = [ah.split(",") for ah in hl_groups]

    fig_arr = []
    ext = os.path.splitext(args_log)[1]
    title = os.path.basename(args_log.replace(profiler_str, ""))
    if run_trace:
        df, fig, fig2 = run_pc_trace(args_log, hl_groups, title, args)
    else:
        df, fig = run_inst_log(args_log, hl_groups, title, args)

    log_path = os.path.realpath(args_log)
    fig_arr.append([log_path, fig])
    if run_trace and fig2 is not None:
        fig_arr.append([log_path.replace(ext, f"_exec{ext}"), fig2])

    if args.save_csv:
        df.to_csv(args_log.replace(ext, "_df.csv"), index=False)

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
