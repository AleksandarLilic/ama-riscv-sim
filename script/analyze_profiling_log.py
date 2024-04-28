import os
import glob
import json
import argparse
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
DEFAULT_HWPM = os.path.join(SCRIPT_PATH, "hw_perf_metrics.json")
PROFILER_ENDS = "_inst_profiler.json"
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
    parser = argparse.ArgumentParser(description="Analyze profiling log")
    parser.add_argument('-l', '--json_log', type=str, help="JSON log with profiling data")
    parser.add_argument('-d', '--json_dir', type=str, help="Directory with JSON logs with profiling data")
    parser.add_argument('--exclude', type=inst_exists, nargs='+', help="Exclude specific instructions")
    parser.add_argument('--exclude_type', type=inst_type_exists, nargs='+', help=f"Exclude specific instruction types. Available types are: {', '.join(inst_t.keys())}")
    parser.add_argument('--top', type=int, help="Number of N most common instructions to display. Default is all.")
    parser.add_argument('--allow_zero', action='store_true', default=False, help="Allow instructions with zero count to be displayed")
    parser.add_argument('--highlight', '--hl', type=str, nargs='+', help="Highlight specific instructions")
    parser.add_argument('-s-', '--silent', action='store_true', help="Don't display chart(s) in pop-up window")
    parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
    parser.add_argument('--save_pdf', type=str, nargs='?', const=os.getcwd(), default=None, help="Save charts as PDF. Saved as a single PDF file if multiple charts are generated. Requires path to save the PDF file. Default is current directory.")
    parser.add_argument('--save_csv', action='store_true', help="Save source data as CSV")
    parser.add_argument('--combined_only', action='store_true', help="Only save combined charts/data. Ignored if single JSON log is provided")
    parser.add_argument('-o', '--output', type=str, default=os.getcwd(), help="Output directory for the combined PNG and CSV. Ignored if single json file is provided. Default is current directory. Standalone PNGs and CSVs will be saved in the same directory as the source json file irrespective of this option.")
    parser.add_argument('--estimate_perf', type=str, nargs='?', const=DEFAULT_HWPM, default=None, help=f"Estimate performance based on instruction count. Requires JSON file with HW performance metrics. Default is '{DEFAULT_HWPM}'. Filtering options (exclude, exclude_type, top, allow_zero) are ignored for performance estimation.")
    
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
    return hwpm["cpu_frequency_mhz"], cpi, exec_time_us, mips

def analyze_log(df, hl_groups, log, args, combined=False):
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
    #suptitle_str += f"\nInstructions profiled: {df['count'].sum()}"
    suptitle_str += f"\nInstructions profiled: {inst_profiled}"
    if args.exclude or args.exclude_type:
        suptitle_str += f" ({df['count'].sum()} shown, "
        suptitle_str += f"{inst_profiled - df['count'].sum()} excluded)"
    if args.estimate_perf and not combined:
        suptitle_str += f"\nEstimated HW performance at {hw_perf[0]}MHz: "
        suptitle_str += f"CPI={hw_perf[1]:.2f}, exec_time={hw_perf[2]:.1f}us, "
        suptitle_str += f"MIPS={hw_perf[3]:.1f}"
    
    fig.suptitle(suptitle_str)
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

def run(args):
    if not args.json_log and not args.json_dir:
        raise ValueError("No JSON file or directory provided")
    
    all_logs = []
    if args.json_log:
        if not os.path.exists(args.json_log):
            raise FileNotFoundError(f"File {args.json_log} not found")
        all_logs.append(args.json_log)
    
    if args.json_dir:
        if not os.path.isdir(args.json_dir):
            raise FileNotFoundError(f"Directory {args.json_dir} not found")
        all_in_dir = glob.glob(os.path.join(args.json_dir, "*" + PROFILER_ENDS))
        if not all_in_dir:
            raise FileNotFoundError(f"No JSON files found in directory " + \
                                    f"{args.json_dir}")
        all_logs.extend(all_in_dir)

    hl_groups = []
    if not (args.highlight == None):
        if len(args.highlight) > len(highlight_colors):
            raise ValueError(f"Too many instructions to highlight " + \
                             f"max is {len(highlight_colors)}, " + \
                             f"got {len(args.highlight)}")
        else:
            hl_groups = [args.highlight.split(",") 
                         for args.highlight in args.highlight]

    if not os.path.exists(args.output):
        raise FileNotFoundError(f"Output directory {args.output} not found")

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

            df = pd.DataFrame(ar, columns=['name', 'count'])
            df = df.sort_values(by='count', ascending=True)
            df_arr.append(df)

            if not args.combined_only:
                fig_arr.append([
                    os.path.realpath(log),
                    analyze_log(df, hl_groups, log, args)
                ])
    
    # combine all the data
    combined_fig = None
    if len(all_logs) > 1:
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
            analyze_log(df_combined, hl_groups, title_combined, args, True)
        ]
        
    if combined_fig:
        combined_out_path = os.path.join(
            args.output, 
            f"{title_combined}{PROFILER_ENDS}".replace(" ", "_")
        )
    
    if args.save_png: # each chart is saved as a separate PNG file
        for name, fig in fig_arr:
            fig.savefig(name.replace(" ", "_").replace(".json", ".png"))
            combined_fig[1].savefig(combined_out_path.replace(".json", ".png"))
    
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
    run(args)
