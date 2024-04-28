import os
import glob
import json
import argparse
import pandas as pd
import matplotlib.pyplot as plt

PROFILER_ENDS = "_inst_profiler.json"
MEM_S = "MEM_S"
MEM_L = "MEM_L"

inst_t_mem = {
    MEM_S: ["sb", "sh", "sw"],
    MEM_L: ["lb", "lh", "lw", "lbu", "lhu"],
}

inst_t = {
    "ARITH": ["add", "sub", "sll", "srl", "sra", "slt", "sltu", "xor", "or", "and", 
              "addi", "slli", "srli", "srai", "slti", "sltiu", "xori", "ori", "andi",
              "lui", "auipc"],
    "MEM": inst_t_mem[MEM_S] + inst_t_mem[MEM_L],
    "BRANCH": ["beq", "bne", "blt", "bge", "bltu", "bgeu"],
    "JUMP": ["jalr", "jal"],
    "CSR": ["csrrw", "csrrs", "csrrc", "csrrwi", "csrrsi", "csrrci"],
    "ENV": ["ecall", "ebreak"],
    "NOP": ["nop"],
    "FENCE": ["fence.i"],
}

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

def parse_args():
    parser = argparse.ArgumentParser(description="Analyze profiling log")
    parser.add_argument('-l', '--json_log', type=str, help="JSON file with profiling data")
    parser.add_argument('-d', '--json_dir', type=str, help="Directory with JSON files with profiling data")
    parser.add_argument('-t', '--top', type=int, help="Number of N most common instructions to display. Default is all.")
    #parser.add_argument('-e', '--exclude', type=str, nargs='+', help="Exclude specific instructions")
    #parser.add_argument('--exclude_type', type=str, nargs='+', help="Exclude specific instruction types")
    #parser.add_argument('-i', '--include', type=str, nargs='+', help="Include only specific instructions")
    #parser.add_argument('--include_type', type=str, nargs='+', help="Include only specific instruction types")
    parser.add_argument('--highlight', '--hi', type=str, nargs='+', help="Highlight specific instructions")
    parser.add_argument('--silent', action='store_true', help="Don't display the chart(s) in pop-up window")
    parser.add_argument('--save_png', action='store_true', help="Save charts as PNG")
    #parser.add_argument('--save_pdf', action='store_true', help="Save charts as PDF")
    #parser.add_argument('--save_csv', action='store_true', help="Save source data as CSV")
    parser.add_argument('--combined_only', action='store_true', help="Only save combined charts/data. Ignored if single json file is provided")
    #parser.add_argument('-o', '--output', type=str, help="Output directory for the combined charts/data. Ignored if single json file is provided. Default is current directory. Standalone charts/data will be saved in the same directory as the source json file.")
    parser.add_argument('--allow_zero', action='store_true', default=False, help="Allow instructions with zero count to be displayed")
    #parser.add_arument('--estimate_perf', type=str, help="Estimate performance based on instruction count. Requires JSON file with HW performance metrics")
    
    return parser.parse_args()

def analyze_log(df, hl_groups, log, args, combined=False):
    title = os.path.basename(log.replace(PROFILER_ENDS, ""))

    # separate the instructions by type
    df['i_type'] = df['name'].map(inst_t_map)
    df_g = df[['i_type', 'count']].groupby('i_type').sum()
    df_g = df_g.sort_values(by='count', ascending=True)

    # separate the memory instructions by type
    df['i_mem_type'] = df['name'].map(inst_t_mem_map)
    df_mem_g = df[['i_mem_type', 'count']].groupby('i_mem_type').sum()
    df_mem_g = df_mem_g.sort_values(by='count', ascending=False)

    if args.top:
        df = df.tail(args.top)
    if not args.allow_zero:
        df = df[df['count'] != 0]

    # add a bar chart
    ROWS = 2
    COLS = 1
    rect = []
    fig, ax = plt.subplots(ROWS, COLS,
                           figsize=(COLS*10, ROWS*(len(df)+len(df_g))/6),
                           height_ratios=(len(df)*.95, len(df_g)),
                           gridspec_kw=find_gs(len(df)+len(df_g)))
    suptitle_str = f"Execution profile for {title}"
    suptitle_str += f"\nInstructions profiled: {df['count'].sum()}"
    #suptitle_str += f"\nEstimated HW perf: ..."
    fig.suptitle(suptitle_str)
    rect.append(ax[0].barh(df['name'], df['count'], color=colors["blue_base"]))
    rect.append(ax[1].barh(df_g.index, df_g['count'], color=colors["blue_base"]))
    y_ax0_offset = min(0.025, len(df)/2_000)
    ax[0].margins(y=0.03-y_ax0_offset)
    ax[1].margins(y=0.03)

    for i in range(ROWS):
        ax[i].bar_label(rect[i], padding=3)
        ax[i].set_xlabel('Count')
        ax[i].grid(axis='x')
        ax[i].margins(x=0.06)

    # highlight specific instructions, if any
    hc = 0
    for hl_inst in hl_groups:
        for i, r in enumerate(rect[0]):
            if df.iloc[i]['name'] in hl_inst:
                r.set_color(highlight_colors[hc])
        hc += 1
    
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

    # handle display and saving
    if not args.silent:
        if combined:
            plt.show()
        else:
            plt.show()
    
    if args.save_png:
        plt.savefig(log.replace(".json", ".png"))
    
    plt.close()

def main():
    args = parse_args()

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
            raise FileNotFoundError(f"No JSON files found in directory {args.json_dir}")
        all_logs.extend(all_in_dir)

    hl_groups = []
    if not (args.highlight == None):
        if len(args.highlight) > len(highlight_colors):
            raise ValueError(f"Too many instructions to highlight, max is {len(highlight_colors)}, got {len(args.highlight)}")
        else:
            hl_groups = [args.highlight.split(",") for args.highlight in args.highlight]


    df_arr = []
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
                analyze_log(df, hl_groups, log, args)
    
    # combine all the data
    if len(all_logs) > 1:
        df_combined = pd.concat(df_arr)
        if 'i_type' in df_combined.columns:
            df_combined = df_combined.drop(columns=['i_type'])
        if 'i_mem_type' in df_combined.columns:
            df_combined = df_combined.drop(columns=['i_mem_type'])
        df_combined = df_combined.groupby('name', as_index=False).sum()
        df_combined = df_combined.sort_values(by='count', ascending=True)
        analyze_log(df_combined, hl_groups, f"all workloads", args, combined=True)

if __name__ == "__main__":
    main()
