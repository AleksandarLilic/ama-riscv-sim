import os
import json
import argparse
import pandas as pd
from analyze_profiling_log import get_base_int_pc

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

df = pd.read_csv(args.log, sep=':', header=None, names=['pc', 'instr'])
df['pc_int'] = df['pc'].apply(get_base_int_pc)
df['bin'] = pd.cut(df['pc_int'], bins=symbols_bins["bins"],
                   labels=symbols_bins["labels"], include_lowest=True)
df = symbol_change(df)

with open(args.log.replace('.log', '_annotated.log'), 'w') as f:
    for index, row in df.iterrows():
        if pd.notna(row['bin_s']):
            f.write(f"<{row['bin_s']}> at {row['pc']}:\n")
        f.write(f"    {row['pc']}:{row['instr']}\n")
