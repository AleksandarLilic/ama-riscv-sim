import os
import sys
import glob
import json
import argparse
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.ticker import MultipleLocator, FuncFormatter, LogFormatter, LogFormatterSciNotation

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
DEFAULT_HWPM = os.path.join(SCRIPT_PATH, "hw_perf_metrics.json")
PROFILER_ENDS = "_profiler.json"
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
CACHE_LINE_BYTES = 64//INST_BYTES
BASE_ADDR = 0x80000000

inst_t_mem = {
    MEM_S: ["sb", "sh", "sw"],
    MEM_L: ["lb", "lh", "lw", "lbu", "lhu"],
}

inst_t = {
    ARITH: ["add", "sub", "sll", "srl", "sra", "slt", "sltu", "xor", "or", "and", 
            "addi", "slli", "srli", "srai", "slti", "sltiu", "xori", "ori", "andi",
            "lui", "auipc"],
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
    "#2a9d8f",
    "#e9c46a",
    "#f4a261",
    "#e76f51",
]

# memory instructions breakdown store vs load
inst_mem_bd = {
    MEM_L : ["Load", colors["blue_light1"]],
    MEM_S : ["Store", colors["blue_light2"]],
}

def find_gs(x):
    prec = 4
    m1 = -0.0058
    b1 = 0.3986
    m2 = 0.0024
    b2 = 0.8092
    return {
        'hspace': round(m1 * x + b1, prec),
        'top': round(m2 * x + b2, prec)
    }

def get_norm_pc(pc):
    return int(pc,16) - int(BASE_ADDR)

def get_count(parts, df):
    pc = get_norm_pc(parts[0].strip())
    count_series  = df.loc[df["pc_real"] == pc, "count"]
    count = count_series.squeeze() if not count_series.empty else 0
    return count, pc

def num_to_hex(val, pos):
    return f"0x{int(val*INST_BYTES):03X}"

def inst_exists(inst):
    if inst not in all_inst:
        raise ValueError(f"Invalid instruction '{inst}'. " + \
                         f"Available instructions are: {', '.join(all_inst)}")
    return inst

def inst_type_exists(inst_type):
    if inst_type not in all_inst_types:
        raise ValueError(f"Invalid instruction type '{inst_type}'. " + \
                         f"Available types are: {', '.join(all_inst_types)}")
    return inst_type

def parse_args():
    parser = argparse.ArgumentParser(description="Analyze profiling instruction and PC log")
    # either
    parser.add_argument('-i', '--inst_log', type=str, nargs='+', help="JSON instruction log with profiling data. Multiple logs can be provided at once without repeating the option. Can be combined with --inst_dir")
    parser.add_argument('--inst_dir', type=str, help="Directory with JSON instruction logs with profiling data")
    # or
    parser.add_argument('-p', '--pc_log', type=str, nargs='+', help="JSON PC log with profiling data. Multiple logs can be provided at once without repeating the option. Can be combined with --pc_dir")
    parser.add_argument('--pc_dir', type=str, help="Directory with JSON PC logs with profiling data")
    
    # instruction log only options
    parser.add_argument('--exclude', type=inst_exists, nargs='+', help="Exclude specific instructions. Instruction log only option")
    parser.add_argument('--exclude_type', type=inst_type_exists, nargs='+', help=f"Exclude specific instruction types. Available types are: {', '.join(inst_t.keys())}. Instruction log only option")
    parser.add_argument('--top', type=int, help="Number of N most common instructions to display. Default is all. Instruction log only option")
    parser.add_argument('--allow_zero', action='store_true', default=False, help="Allow instructions with zero count to be displayed. Instruction log only option")
    parser.add_argument('--highlight', '--hl', type=str, nargs='+', help="Highlight specific instructions. Instruction log only option")
    parser.add_argument('--estimate_perf', type=str, nargs='?', const=DEFAULT_HWPM, default=None, help=f"Estimate performance based on instruction count. Requires JSON file with HW performance metrics. Default is '{DEFAULT_HWPM}'. Filtering options (exclude, exclude_type, top, allow_zero) are ignored for performance estimation. Instruction log only option")
    
    # pc log only options
    parser.add_argument('--dasm', type=str, help="Disassembly dasm file to backannotate the PC log. PC log only option")
    #parser.add_argument('--pc_begin', type=str, help="Start PC address to filter the PC log. PC log only option")
    #parser.add_argument('--pc_end', type=str, help="End PC address to filter the PC log. PC log only option")

    # common options
    parser.add_argument('-s', '--silent', action='store_true', help="Don't display chart(s) in pop-up window")
    parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
    parser.add_argument('--save_pdf', type=str, nargs='?', const=os.getcwd(), default=None, help="Save charts as PDF. Saved as a single PDF file if multiple charts are generated. Requires path to save the PDF file. Default is current directory.")
    parser.add_argument('--save_csv', action='store_true', help="Save source data as CSV")
    parser.add_argument('--combined_only', action='store_true', help="Only save combined charts/data. Ignored if single JSON log is provided")
    parser.add_argument('-o', '--output', type=str, default=os.getcwd(), help="Output directory for the combined PNG and CSV. Ignored if single json file is provided. Default is current directory. Standalone PNGs and CSVs will be saved in the same directory as the source json file irrespective of this option.")
    
    return parser.parse_args()

def estimate_perf(df, hwpm):
    expected_metrics = ["cpu_frequency_mhz", "pipeline_latency", 
                        "branch_latency", "jump_latency"]
    # check if all expected metrics are present
    for metric in expected_metrics:
        if metric not in hwpm:
            raise ValueError(f"Missing metric '{metric}' in " + \
                              "HW performance metrics JSON file")
    
    branches = df[df['i_type'] == BRANCH]['count'].sum()
    jumps = df[df['i_type'] == JUMP]['count'].sum()
    all_inst = df['count'].sum()
    other_inst = all_inst - branches - jumps
    cycles = hwpm["pipeline_latency"] + other_inst + \
             branches * hwpm["branch_latency"] + \
             jumps * hwpm["jump_latency"]
    cpi = cycles / all_inst
    cpu_period = 1 / (hwpm["cpu_frequency_mhz"])
    exec_time_us = cycles * cpu_period
    mips = all_inst / exec_time_us
    return hwpm["cpu_frequency_mhz"], cycles, cpi, exec_time_us, mips

def analyze_inst_log(df, hl_groups, log, args, combined=False):
    title = os.path.basename(log.replace(PROFILER_ENDS, ""))
    inst_profiled = df['count'].sum()
    df['i_type'] = df['name'].map(inst_t_map)
    df['i_mem_type'] = df['name'].map(inst_t_mem_map)

    if args.estimate_perf:
        with open(args.estimate_perf, 'r') as file:
            hw_perf_metrics = json.load(file)
            hw_perf = estimate_perf(df, hw_perf_metrics)

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
                           gridspec_kw=find_gs(len(df)+len(df_g)))
    suptitle_str = f"Execution profile for {title}"
    suptitle_str += f"\nInstructions profiled: {inst_profiled}"
    if args.exclude or args.exclude_type:
        suptitle_str += f" ({df['count'].sum()} shown, "
        suptitle_str += f"{inst_profiled - df['count'].sum()} excluded)"
    if args.estimate_perf and not combined:
        suptitle_str += f"\nEstimated HW performance at {hw_perf[0]}MHz: "
        suptitle_str += f"cycles = {hw_perf[1]:.0f}, CPI={hw_perf[2]:.2f}, exec time={hw_perf[3]:.1f}us, "
        suptitle_str += f"MIPS={hw_perf[4]:.1f}"
    
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
    for hl_inst in hl_groups:
        for i, r in enumerate(box[0]):
            if df.iloc[i]['name'] in hl_inst:
                r.set_color(highlight_colors[hc])
        hc += 1
    
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

def run_inst_log(ar, hl_groups, log, args):
    df = pd.DataFrame(ar, columns=['name', 'count'])
    df = df.sort_values(by='count', ascending=True)
    fig = None
    if not args.combined_only:
        fig = analyze_inst_log(df, hl_groups, log, args)
    return df, fig

def backannotate_dasm(log, df):
    symbols = {}
    dasm_ext = os.path.splitext(log)[1]
    with open(log, 'r') as infile, \
    open(log.replace(dasm_ext, '.prof' + dasm_ext), 'w') as outfile:
        current_symbol = None
        append = False
        NUM_DIGITS = len(str(df['count'].max())) + 1
        
        for line in infile:
            if line.startswith('Disassembly of section .text:'):
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
                    pc_start = get_norm_pc(parts[0].strip())
                    symbol_name = parts[1][1:-1] # remove <> from symbol name
                    if current_symbol:
                        symbols[current_symbol]['pc_end'] = hex(previous_pc)
                    current_symbol = symbol_name
                    symbols[current_symbol] = {
                        'pc_start': hex(pc_start),
                        "pc_end": None,
                        "acc_count": 0
                    }
                    previous_pc = pc_start
                
                if len(parts) == 2 and parts[1].startswith('\t'):
                    # detected instruction
                    count, previous_pc = get_count(parts, df)
                    outfile.write("{:{}} {}".format(count, NUM_DIGITS, line))
                    symbols[current_symbol]['acc_count'] += count
                
                else: # not an instruction
                    outfile.write(line)
                
            else: # not .text section
                outfile.write(line)
        
        # write the last symbol
        symbols[current_symbol]['pc_end'] = hex(previous_pc)
    
    return symbols

def run_pc_log(ar, log, args):
    title = os.path.basename(log.replace(PROFILER_ENDS, ""))
    df = pd.DataFrame(ar, columns=['pc', 'count'])
    df['pc'] = df['pc'].astype(int)
    df['pc_hex'] = df['pc'].apply(lambda x: f'{x:08x}')
    df['pc_real'] = df['pc'] * INST_BYTES
    df['pc_real_hex'] = df['pc_real'].apply(lambda x: f'{x:08x}')

    # TODO: consider option to reverse the PC log so it grows down from the top
    # like .dump does

    # slice df based on the input
    # TODO filtering as a feature, has to be taken into account when 
    # labeling the chart and when backannotating the .dasm file
    #df = df.loc[
    #    (df.pc >= (int("0x220", 16)//4)) & 
    #    (df.pc < (int("0x300", 16)//4))
    #]

    symbols = {}
    if args.dasm:
        symbols = backannotate_dasm(args.dasm, df)

    # TODO: should probably limit the number of PC entries to display or 
    # auto enlarge the figure size (but up to a point, then limit anyways)
    gs = {'top': 0.93, 'bottom': 0.07}
    fig, ax = plt.subplots(figsize=(12,15), gridspec_kw=gs)
    ax.barh(df['pc'], df['count'], height=1, alpha=0.8)
    #       , edgecolor=(0, 0, 0, 0.1))
    ax.set_xscale('log')

    # update axis
    #formatter = LogFormatter(base=10, labelOnlyBase=True)
    formatter = LogFormatterSciNotation(base=10)
    ax.xaxis.set_major_formatter(formatter)
    ax.yaxis.set_major_locator(MultipleLocator(32//INST_BYTES))
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

    # add cache line coloring
    # FIXME when filtering is implemented, assumes exec starts from 0 for now
    for i in range(df.pc.min(), df.pc.max(), CACHE_LINE_BYTES):
        color = 'green' if (i//CACHE_LINE_BYTES) % 2 == 0 else 'white'
        ax.axhspan(i, i+CACHE_LINE_BYTES, color=color, alpha=0.1)

    # add lines for symbols, if any
    text_offset = round(df.pc.max()/160,1)
    for sym in symbols:
        pc_start = int(symbols[sym]['pc_start'], 16)//INST_BYTES
        pc_end = int(symbols[sym]['pc_end'], 16)//INST_BYTES
        ax.axhline(y=pc_start-.5, color='black', linestyle='-', alpha=0.5)
        ax.text(df['count'].max()/4, pc_start + text_offset,
                f"^ {sym} ({symbols[sym]['acc_count']})", color='black',
                fontsize=9, ha='left', va='center',
                bbox=dict(facecolor='white', alpha=0.6, linewidth=0, pad=1)
                )

    if symbols:
        # add line for the last symbol
        ax.axhline(y=pc_end+.5, color='black', linestyle='-', alpha=0.5)    

    return df, fig

def run_main(args):
    inst_args = [args.inst_log, args.inst_dir]
    pc_args = [args.pc_log, args.pc_dir]
    run_inst = any(inst_args)
    run_pc = any(pc_args)

    if run_inst and run_pc:
        raise ValueError("Inst and pc options cannot be mixed")

    if not run_inst and not run_pc:
        raise ValueError("No JSON instruction or PC log provided")

    if not os.path.exists(args.output):
        raise FileNotFoundError(f"Output directory {args.output} not found")
    
    if run_pc:
        args_log = args.pc_log
        args_dir = args.pc_dir
        profiler_str = "pc-cnt"
    else:
        args_log = args.inst_log
        args_dir = args.inst_dir
        profiler_str = "inst"
    
    all_logs = []
    if args_log:
        for l in args_log:
            if not os.path.exists(l):
                raise FileNotFoundError(f"File {l} not found")
            all_logs.append(l)
    
    if args_dir:
        if not os.path.isdir(args_dir):
            raise FileNotFoundError(f"Directory {args_dir} not found")
        all_in_dir = glob.glob(os.path.join(args_dir, 
                                            "*" + profiler_str + PROFILER_ENDS))
        if not all_in_dir:
            raise FileNotFoundError(f"No JSON files found in directory " + \
                                    f"{args_dir}")
        all_logs.extend(all_in_dir)

    if len(all_logs) > 1 and args.dasm:
        raise ValueError("Cannot backannotate multiple PC logs at once. " + \
                         "Provide a single PC log file to backannotate or " + \
                         "remove the --dasm option")
    
    if run_inst:
        hl_groups = []
        if not (args.highlight == None):
            if len(args.highlight) > len(highlight_colors):
                raise ValueError(f"Too many instructions to highlight " + \
                                f"max is {len(highlight_colors)}, " + \
                                f"got {len(args.highlight)}")
            else:
                hl_groups = [args.highlight.split(",") 
                            for args.highlight in args.highlight]

    df_arr = []
    fig_arr = []
    for log in all_logs:
        with open(log, 'r') as file:
            data = json.load(file)

            ar = []
            for key in data:
                if key.startswith('_'): # skip internal keys
                    continue
                ar.append([key, data[key]['count']])

            if run_inst:
                df, fig = run_inst_log(ar, hl_groups, log, args)
            else:
                df, fig = run_pc_log(ar, log, args)
            
            df_arr.append(df)
            if not args.combined_only:
                fig_arr.append([os.path.realpath(log),fig])

    # combine all the data
    combined_fig = None
    if len(all_logs) > 1 and run_inst:
        title_combined = "all_workloads"
        df_combined = pd.concat(df_arr)
        if 'i_type' in df_combined.columns:
            df_combined = df_combined.drop(columns=['i_type'])
        if 'i_mem_type' in df_combined.columns:
            df_combined = df_combined.drop(columns=['i_mem_type'])
        df_combined = df_combined.groupby('name', as_index=False).sum()
        df_combined = df_combined.sort_values(by='count', ascending=True)
        combined_fig = [
            f"{title_combined}{PROFILER_ENDS}",
            analyze_inst_log(df_combined, hl_groups, title_combined, args, True)
        ]
        
    if combined_fig:
        combined_out_path = os.path.join(
            args.output, 
            f"{title_combined}{PROFILER_ENDS}".replace(" ", "_")
        )
    
    if args.save_png: # each chart is saved as a separate PNG file
        for name, fig in fig_arr:
            fig.savefig(name.replace(" ", "_").replace(".json", ".png"))
            if combined_fig:
                combined_fig[1].savefig(combined_out_path.replace(".json", 
                                                                  ".png"))
    
    if combined_fig: # add combined chart as the first one in the list (ie pdf)
        fig_arr.insert(0, combined_fig)
    
    if args.save_pdf: # all charts are saved in a single PDF file
        pdf_path =  os.path.join(args.save_pdf, 
                                 PROFILER_ENDS[1:].replace(".json", ".pdf"))
        with PdfPages(pdf_path) as pdf:
            total_pages = len(fig_arr)
            page = 1
            for name, fig in fig_arr:
                fig.supxlabel(f"Page {page} of {total_pages}", 
                              x=0.93, fontsize=10)
                page += 1
                pdf.savefig(fig)
    
    if args.save_csv: # each log is saved as a separate CSV file
        for log, df in zip(all_logs, df_arr):
            df.to_csv(log.replace(".json", ".csv"), index=False)
        
        if len(all_logs) > 1: # and also saved as a combined CSV file
            df_combined.to_csv(combined_out_path.replace(".json", ".csv"),
                               index=False)
    
if __name__ == "__main__":
    args = parse_args()
    run_main(args)
