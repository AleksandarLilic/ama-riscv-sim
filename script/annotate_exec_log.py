import os
import re
import json
import argparse
import pandas as pd
from run_analysis import get_base_int_addr

def symbol_change(df):
    # e.g. jump or branch to a function
    df['bin_s'] = df['bin'].where(df['bin'] != df['bin'].shift())
    return df

def file_exists(file):
    if not os.path.exists(file):
        raise ValueError(f"File {file} does not exist")
    return file

parser = argparse.ArgumentParser(description="Annotate execution log")
parser.add_argument("-l", "--log", required=True, type=file_exists, help="Execution log file")
parser.add_argument("-s", "--symbols", required=True, type=file_exists, help="Symbols file")

args = parser.parse_args()

with open(args.symbols, 'r') as f:
    symbols = json.load(f)

symbols_bins = {"bins": [], "labels": []}
symbols_bins["bins"].append((symbols[list(symbols.keys())[0]]['pc_start_real']))
for symbol, data in symbols.items():
    symbols_bins["bins"].append(data['pc_end_real'])
    symbols_bins["labels"].append(symbol)

with open(args.log, 'r') as f:
    lines = f.readlines()

pattern = re.compile(r'^[0-9a-fA-F]+:')
lines_pc = [l for l in lines if pattern.match(l)]

df = pd.DataFrame([x.split(':', 1) for x in lines_pc], columns=['pc', 'instr'])
df['pc_int'] = df['pc'].apply(get_base_int_addr)
df['bin'] = pd.cut(df['pc_int'], bins=symbols_bins["bins"],
                   labels=symbols_bins["labels"], include_lowest=True)
df = symbol_change(df)

symbols = [f"<{row['bin_s']}> at {row['pc']}:\n" if pd.notna(row['bin_s'])
           else "" for index, row in df.iterrows()]

indent = "    "
out_lines = []
for pc_l in lines_pc:
    l = lines.pop(0)
    while l != pc_l:
        out_lines.append(indent + l)
        l = lines.pop(0)
    out_lines.append(symbols.pop(0))
    out_lines.append(indent + l)

with open(args.log.replace('.log', '_annotated.log'), 'w') as f:
    f.writelines(out_lines)
