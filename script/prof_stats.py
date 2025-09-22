#!/usr/bin/env python3

import argparse
import os
import sys
from collections import OrderedDict, defaultdict
from typing import Any, Dict, Iterable, List, Optional, Tuple

import matplotlib.pyplot as plt
import pandas as pd


def parse_folded_line(line: str) -> Tuple[List[str], int]:
    """
    Parse one line of folded callstack:
    e.g. "call_main;main;Func_1; 100"
    Returns (frames_list, count_int)
    """
    line = line.strip()
    if not line:
        return None, None

    # split off the count (last whitespace-separated token)
    try:
        stack_str, count_str = line.rsplit(None, 1)
    except ValueError:
        raise ValueError(
            f"Line not in expected format (no count found): {line!r}")

    try:
        count = int(count_str)
    except ValueError:
        raise ValueError(
            f"Sample count is not integer: {count_str!r} in line: {line!r}")

    # split frames by ';'
    frames = [f for f in stack_str.split(';') if f]
    if not frames:
        raise ValueError(f"No frames found in line: {line!r}")
    return frames, count

def accumulate_samples(lines: Iterable[str]) -> Tuple[pd.DataFrame, int]:
    self_counts = defaultdict(int)
    total_counts = defaultdict(int)
    total_samples = 0
    for lineno, line in enumerate(lines, start=1):
        stripped = line.strip()
        if not stripped:
            continue
        frames, count = parse_folded_line(stripped)
        if frames is None:
            continue

        total_samples += count
        # leaf is last frame
        leaf = frames[-1]
        self_counts[leaf] += count

        # attribute to every frame on the path
        for func in frames:
            total_counts[func] += count

    # build df
    data = []
    for func, self_c in self_counts.items():
        data.append({
            'symbol': func,
            'self_counts': self_c,
            'total_counts': total_counts.get(func, 0),
        })
    df = pd.DataFrame(data, columns=['symbol', 'self_counts', 'total_counts'])
    df.attrs['total_samples'] = total_samples
    return df

def format_flat_profile(
    df: pd.DataFrame,
    top_n: Optional[int] = None,
    clk: Optional[float] = None,
) -> pd.DataFrame:
    smpls = df.attrs['total_samples']
    if smpls <= 0:
        raise ValueError("total_samples must be > 0")

    work = df.copy()
    work['percent'] = work['self_counts'] / float(smpls) * 100.0

    work = work.sort_values(
        by=['self_counts', 'symbol'], ascending=[False, True], kind='mergesort')
    work['cum_percent'] = work['percent'].cumsum()

    if clk is not None and clk > 0:
        # clk in MHz -> sample time in microseconds
        sample_time_us = 1.0 / float(clk)
        work.attrs['clk_mhz'] = clk
        work['self_time'] = work['self_counts'].astype(float) * sample_time_us
        work['total_time'] = work['total_counts'].astype(float) * sample_time_us
        work.attrs['total_time'] = float(smpls) * sample_time_us

        unit_chain = ['us', 'ms', 's']
        unit_idx = 0
        max_self = float(work['self_time'].max()) if not work.empty else 0.0
        LIM = 1_000_000
        while (max_self > LIM) and (unit_idx < len(unit_chain) - 1):
            work['self_time'] /= 1000.0
            work['total_time'] /= 1000.0
            work.attrs['total_time'] /= 1000.0
            unit_idx += 1
            max_self = float(work['self_time'].max()) if not work.empty else 0.0
        work.attrs['time_unit'] = unit_chain[unit_idx]
    else:
        # Ensure no stale unit lingers
        work.attrs.pop('time_unit', None)

    if top_n is not None:
        work = work.head(top_n)

    return work

def print_profile(df: pd.DataFrame, merged=False) -> None:
    cycles_trace = ('self_time' in df.columns) and ('total_time' in df.columns)
    time_unit = df.attrs.get('time_unit') if cycles_trace else None

    hdr_cycles = "{:>6} {:>15} {:>10} {:>10} {:>10} {:>10}".format(
        "%[c]", "cumulative[c]", "self[c]", "total[c]",
        f"self[{time_unit}]", f"total[{time_unit}]")
    hdr_inst = "{:>6} {:>15} {:>10} {:>10}".format(
        "%[i]", "cumulative[i]", "self[i]", "total[i]")
    hdr = "{:>6} {:>15} {:>10} {:>10}".format(
        "%", "cumulative", "self", "total")

    if merged:
        print("{}   {} {:>7} {:>7}   {}".format(
            hdr_inst, hdr_cycles, "ipc", "cpi", "symbol"))
        for row in df.itertuples(index=False):
            print(
                "{:6.2f} {:15.2f} {:10d} {:10d}   {:6.2f} {:15.2f} {:10d} {:10d} {:10.1f} {:10.1f} {:7.3f} {:7.3f}   {}".format(
                float(getattr(row, 'percent_inst')),
                float(getattr(row, 'cum_percent_inst')),
                int(getattr(row, 'self_counts_inst')),
                int(getattr(row, 'total_counts_inst')),
                float(getattr(row, 'percent_cycle')),
                float(getattr(row, 'cum_percent_cycle')),
                int(getattr(row, 'self_counts_cycle')),
                int(getattr(row, 'total_counts_cycle')),
                float(getattr(row, 'self_time')),
                float(getattr(row, 'total_time')),
                float(getattr(row, 'ipc')),
                float(getattr(row, 'cpi')),
                getattr(row, 'symbol'),
            ))
        return

    if cycles_trace:
        print("{}   {}".format(hdr_cycles, "symbol"))
        for row in df.itertuples(index=False):
            print(
                "{:6.2f} {:15.2f} {:10d} {:10d} {:10.1f} {:10.1f}   {}".format(
                float(getattr(row, 'percent')),
                float(getattr(row, 'cum_percent')),
                int(getattr(row, 'self_counts')),
                int(getattr(row, 'total_counts')),
                float(getattr(row, 'self_time')),
                float(getattr(row, 'total_time')),
                getattr(row, 'symbol'),
            ))

    else:
        print("{}   {}".format(hdr, "symbol"))
        for row in df.itertuples(index=False):
            print("{:6.2f} {:15.2f} {:10d} {:10d}   {}".format(
                float(getattr(row, 'percent')),
                float(getattr(row, 'cum_percent')),
                int(getattr(row, 'self_counts')),
                int(getattr(row, 'total_counts')),
                getattr(row, 'symbol'),
            ))

def run_trace(
    lines,
    top_n: Optional[int] = 32,
    clk: Optional[float] = None
) -> pd.DataFrame:

    try:
        df = accumulate_samples(lines)
    except ValueError as e:
        print(f"Error parsing input: {e}", file=sys.stderr)
        sys.exit(1)

    if df.attrs['total_samples'] == 0:
        print("No samples found.", file=sys.stderr)
        sys.exit(1)

    return(format_flat_profile(df, top_n=top_n, clk=clk))

def draw_single_plot(df, ax, args):
    data = df[args['data']].tolist()
    symbols = df['symbol'].tolist()
    box = ax.barh(symbols, data, color='#7ed3ab')
    if args.get('fmt', None):
        ax.bar_label(box, fmt=args['fmt'], padding=3)
    ax.set_xlim(0, ax.get_xlim()[1] * 1.1)
    ax.set_xlabel(args['xlabel'])
    if args.get('use_ylabel', False):
        ax.set_ylabel("symbol")
    ax.set_title(args['title'])
    return ax

PROF_EVENTS = [
    "inst",
    "cycle",
    "branch",
    "mem",
    "simd",
    "icache_reference",
    "icache_miss",
    "dcache_reference",
    "dcache_miss",
    "bp_mispredict"
]

CLK_DEFAULT = 100.0 # MHz
FIG_H = 3.3

def main():
    parser = argparse.ArgumentParser(description="Convert folded callstack samples to a flat-profile summary.")
    parser.add_argument("-t", "--trace", type=str, required=True, help="Callstack trace. Normally run standalone. Can be combined with -s for inst/cycle only to get detailed execution breakdown")
    parser.add_argument("-s", "--second_trace", type=str, help="Second callstack trace. Used to combine with -t for inst/cycle only to get detailed execution breakdown")
    parser.add_argument("-e", "--event", type=str, choices=PROF_EVENTS, default=PROF_EVENTS[0], help="Callstack sample event type. If both -t and -s are specified, this is ignored and 'inst' is used for -t, and 'cycle' for -s")
    parser.add_argument("-p", "--top", type=int, default=None, help="Show only the top N symbols (functions) by self-samples")
    parser.add_argument("-c", "--clk", type=float, default=CLK_DEFAULT, help="Clock frequency in MHz. Only used for 'cycle' event")
    parser.add_argument("--ipc", action='store_true', default=False, help="Plot IPC instead of CPI for the combined chart. Ignored if -t and -s are not both specified")
    parser.add_argument("--plot", action='store_true', default=False, help="Show plot")
    args = parser.parse_args()

    if (args.second_trace and args.event != PROF_EVENTS[0]):
        parser.error(
            "When both -i and -r are specified, event type shouldn't be set;"
            " auto-defaults to 'inst' for -i and 'cycle' for -r")

    if args.second_trace:
        args.event = PROF_EVENTS[0] # force to 'inst' for -i
        fig, ax = plt.subplots(
            1, 3, figsize=(15, FIG_H), tight_layout=True, sharey=True)
    else:
        fig, ax = plt.subplots(1, 1, figsize=(6, FIG_H), tight_layout=True)

    if not os.path.isfile(args.trace):
        parser.error(f"Trace callstack not found: {args.trace}")
    t_lines = open(args.trace, "r").readlines()
    t_df = run_trace(
        t_lines, clk=args.clk if args.event == PROF_EVENTS[1] else None)
    if args.top is not None:
        t_df = t_df.head(args.top)
    title = 'Profile' + (f' - {args.event.capitalize()}')

    if not args.second_trace:
        print(title)
        print_profile(t_df)
        for atn,atv in t_df.attrs.items():
            print(atn, ":", atv)
            if args.event != PROF_EVENTS[1]:
                # only total_samples is printed if event is not 'cycle'
                break

    a = ax[0] if args.second_trace else ax
    draw_single_plot(t_df, a, {
        'data': 'percent',
        'xlabel': f'% of {args.event} samples',
        'title': title,
        'use_ylabel': True,
        'fmt': '%.1f%%'
    })
    df_len = t_df.index.size

    # resize only height, keep width
    w, h = fig.get_size_inches()
    fig.set_size_inches(w, max(FIG_H, df_len * 0.33))

    if args.second_trace:
        args.event = PROF_EVENTS[1] # force to 'cycle' for -r
        if not os.path.isfile(args.second_trace):
            parser.error(
                f"Second trace callstack not found: {args.second_trace}")
        s_lines = open(args.second_trace, "r").readlines()
        s_df = run_trace(s_lines, clk=args.clk)
        if args.top is not None:
            s_df = s_df.head(args.top)

        draw_single_plot(s_df, ax[1], {
            'data': 'percent',
            'xlabel': f'% of {args.event} samples',
            'title': 'Profile - Cycles',
            'fmt': '%.1f%%'
        })

        # merge two dfs on symbol
        merged = pd.merge(
            t_df, s_df, on='symbol', how='outer', suffixes=('_inst', '_cycle'))
        merged['ipc'] = merged.apply(
            lambda row: (row['total_counts_inst'] / row['total_counts_cycle'])
            if row['total_counts_cycle'] > 0 else None, axis=1)
        merged['ipc'] = merged['ipc'].round(3)
        merged['cpi'] = merged.apply(
            lambda row: (row['total_counts_cycle'] / row['total_counts_inst'])
            if row['total_counts_inst'] > 0 else None, axis=1)
        merged['cpi'] = merged['cpi'].round(3)

        # copy over attrs
        instrs = t_df.attrs['total_samples']
        cycles = s_df.attrs['total_samples']
        merged.attrs["total_instructions"] = instrs
        merged.attrs["total_cycles"] = cycles
        merged.attrs["total_time"] = s_df.attrs['total_time']
        merged.attrs["clk_MHz"] = s_df.attrs['clk_mhz']
        merged.attrs["time_unit"] = s_df.attrs['time_unit']
        merged.attrs["CPI"] = round((cycles/instrs), 3) if instrs > 0 else None
        merged.attrs["IPC"] = round((instrs/cycles), 3) if cycles > 0 else None

        print("Profile - Inst/Cycles combined ")
        print_profile(merged, merged=True)
        for atn,atv in merged.attrs.items():
            print(atn.replace("_", " "), ":", atv)

        d = ["cpi", "ipc"]
        t = ["Cycles Per Instruction (CPI)", "Instructions Per Cycle (IPC)"]
        draw_single_plot(merged, ax[2], {
            'data': d[int(args.ipc)],
            'xlabel': d[int(args.ipc)].upper(),
            'title': t[int(args.ipc)],
            'fmt': '%.3f'
        })

    if args.plot:
        plt.show()
    plt.close('all')

if __name__ == "__main__":
    main()
