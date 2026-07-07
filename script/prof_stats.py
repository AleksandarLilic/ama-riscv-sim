#!/usr/bin/env python3

import argparse
import os
import sys
from collections import OrderedDict, defaultdict
from typing import Any, Dict, Iterable, List, Optional, Tuple

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.ticker import EngFormatter
from utils import INDENT, print_file_saved

SYM_MAXLEN = 40 # y-axis char limit, longer names get a '...' in the middle

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

def format_flat_profile(df: pd.DataFrame, clk: Optional[float] = None) \
-> pd.DataFrame:

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

    return work

def print_profile(
    df: pd.DataFrame, merged=False, labels=("a", "b"), rlabel="ratio") -> None:

    cycles_trace = ('self_time' in df.columns) and ('total_time' in df.columns)
    time_unit = df.attrs.get('time_unit') if cycles_trace else None

    hdr_cycles = "{:>6} {:>15} {:>10} {:>10} {:>10} {:>10}".format(
        "%[c]", "%cumulative[c]", "self[c]", "total[c]",
        f"self[{time_unit}]", f"total[{time_unit}]")
    hdr = "{:>6} {:>15} {:>10} {:>10}".format(
        "%", "%cumulative", "self", "total")

    if merged:
        # a/b are the two traces (first/second -t); legend maps them to labels
        la, lb = labels
        print(f"  a: {la}   b: {lb}   ratio: {rlabel}")
        hdr_a = "{:>6} {:>15} {:>10} {:>10}".format(
            "%[a]", "%cumulative[a]", "self[a]", "total[a]")
        hdr_b = "{:>6} {:>15} {:>10} {:>10}".format(
            "%[b]", "%cumulative[b]", "self[b]", "total[b]")
        print("{}   {} {:>8}   {}".format(hdr_a, hdr_b, "ratio", "symbol"))
        for row in df.itertuples(index=False):
            ratio = getattr(row, 'ratio')
            ratio = float(ratio) if ratio is not None else float('nan')
            print(
                "{:6.2f} {:15.2f} {:10d} {:10d}   "
                "{:6.2f} {:15.2f} {:10d} {:10d} {:8.3f}   {}".format(
                float(getattr(row, 'percent_a')),
                float(getattr(row, 'cum_percent_a')),
                int(getattr(row, 'self_counts_a')),
                int(getattr(row, 'total_counts_a')),

                float(getattr(row, 'percent_b')),
                float(getattr(row, 'cum_percent_b')),
                int(getattr(row, 'self_counts_b')),
                int(getattr(row, 'total_counts_b')),
                ratio,

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

def run_trace(lines, clk: Optional[float] = None) -> pd.DataFrame:
    try:
        df = accumulate_samples(lines)
    except ValueError as e:
        print(f"Error parsing input: {e}", file=sys.stderr)
        sys.exit(1)

    if df.attrs['total_samples'] == 0:
        print("No samples found.", file=sys.stderr)
        sys.exit(1)

    return(format_flat_profile(df, clk=clk))

def save_flat_csv(df: pd.DataFrame, path: str, labels, rlabel=None) -> None:
    # round floats (ratio 3dp, rest 2dp) and tag columns for csv
    # combined -> the two count blocks carry their event label
    # (_a/_b -> _<labelA>/_<labelB>)
    # single -> the one event label; time -> _<unit>
    df = df.copy()
    round_map = {
        c: (3 if c == "ratio" else 2)
        for c in df.columns
        if c == "ratio" or pd.api.types.is_float_dtype(df[c])
    }
    df = df.round(round_map)

    unit = df.attrs.get("time_unit", "us")
    rename = {}
    rename["ratio"] = rlabel if rlabel else "ratio"
    if len(labels) == 2: # combined
        la, lb = labels
        for c in df.columns:
            if c.endswith("_a"):
                rename[c] = c[:-len("_a")] + f"_{la}"
            elif c.endswith("_b"):
                rename[c] = c[:-len("_b")] + f"_{lb}"
    else: # single trace
        tag = labels[0]
        tagged = ("self_counts", "total_counts", "percent", "cum_percent")
        for c in df.columns:
            if c in ("self_time", "total_time"):
                rename[c] = f"{c}_{unit}"
            elif c in tagged:
                rename[c] = f"{c}_{tag}"
    df = df.rename(columns=rename)

    df.to_csv(path, index=False)
    print_file_saved("Flat profile", path)

def label_from_path(p):
    # single-trace label:
    # strip the generic 'callstack_folded_' prefix, keeping the event (+ source)
    # host 'callstack_folded_<ev>_host' -> '<ev>_host',
    # guest/risc-v 'callstack_folded_<ev>[_cosim]' -> '<ev>[_cosim]'
    stem = os.path.splitext(os.path.basename(p))[0]
    pre = "callstack_folded_"
    return stem[len(pre):] if stem.startswith(pre) else stem

def labels_from_paths(paths):
    # for two traces, strip the common leading/trailing name tokens so only the
    # distinguishing part remains
    # (host & guest both reduce to '<ev>',
    # shared source tags like _host/_cosim fall away as common suffix)
    if len(paths) == 1:
        return [label_from_path(paths[0])]
    # a/b arrays of the callstack name without ext
    a, b = [os.path.splitext(os.path.basename(p))[0].split('_') for p in paths]

    # match elements from the front, then back
    i, j = 0, 0
    while (i < len(a)) and (i < len(b)) and (a[i] == b[i]):
        i += 1
    while (j < len(a) - i) and (j < len(b) - i) and (a[-1 - j] == b[-1 - j]):
        j += 1

    # strip away
    la = '_'.join(a[i:len(a) - j])
    lb = '_'.join(b[i:len(b) - j])

    # la/lb if non-empty, otherwise reconstruct a/b from array back to string
    return [la or '_'.join(a), lb or '_'.join(b)]

def shorten_symbol(s, maxlen=SYM_MAXLEN):
    # keep the namespace/class head and the ::method tail
    if len(s) <= maxlen:
        return s
    keep = (maxlen - 3) # room for the '...'
    tail = (keep // 3) # one 3rd from the end
    head = (keep - tail) # and two 3rds from the start
    return s[:head] + "..." + s[-tail:]

def draw_single_plot(df, ax, args):
    data = df[args['data']].tolist()[::-1]
    symbols = df['symbol'].tolist()[::-1]
    box = ax.barh(symbols, data, color='#7ed3ab')
    # relabel ticks with shortened names
    ax.set_yticks(range(len(symbols)))
    ax.set_yticklabels([shorten_symbol(s) for s in symbols])
    f = args.get('fmt', None)
    if f:
        ax.bar_label(box, fmt=f, padding=3)
        # match format on axis if needed
        if args.get('eng_fmt', None):
            ax.xaxis.set_major_formatter(EngFormatter(places=1, sep=""))

    ax.set_xlim(0, ax.get_xlim()[1] * 1.14) # make room for data labels
    ax.set_xlabel(args['xlabel'])
    if args.get('use_ylabel', False):
        ax.set_ylabel("symbol")
    ax.set_title(args['title'])
    return ax

FIG_H = 3.3
FIG_W_1 = 7
FIG_W_3 = (FIG_W_1 * 3) - 2

def main():
    parser = argparse.ArgumentParser(description="Convert folded callstack samples to a flat-profile summary. One trace -> flat profile; two traces -> combined view with a derived ratio (e.g. inst/cycle = IPC).")
    parser.add_argument("-t", "--trace", type=str, nargs='+', required=True, help="One or two folded callstack traces. Two -> combined view with a derived third column (ratio of the two).")
    parser.add_argument("-e", "--event", type=str, nargs='+', default=None, help="Display label(s) for the trace(s); count must match -t. Default: derived from each filename (<name>_folded_host_<ev>.txt -> <ev>).")
    parser.add_argument("--top", type=int, default=None, help="Show only the top N symbols (functions) by self-samples")
    parser.add_argument("--thr", type=int, default=1, help="Show only symbols above this self-%% threshold (in either trace, for combined mode)")
    parser.add_argument("--sort", type=int, default=1, metavar="N", help="Combined mode: rank rows by |N| = 1 first trace, 2 second trace, 3 ratio; sign sets direction (negative = ascending). Default 1 (first trace, descending)")
    parser.add_argument("--clk", type=float, default=None, help="Clock frequency in MHz. Opt-in, single-trace only: when given, adds time columns (count/clk). Only meaningful if -e is cycles. No effect in combined mode.")
    parser.add_argument("--plot", action='store_true', default=False, help="Show plot")
    parser.add_argument("--plot_abs", action='store_true', default=False, help="Plot absolute self counts instead of percentages")
    parser.add_argument("--rinvert", action='store_true', default=False, help="Invert the derived ratio (first/second instead of second/first)")
    parser.add_argument("--rpct", action='store_true', default=False, help="Scale the derived ratio by 100 and label it a percentage (e.g. miss rate, SIMD share)")
    parser.add_argument("--rcomplement", action='store_true', default=False, help="Use '1-ratio' (applied before --rpct scaling), e.g. turn a miss rate into a hit rate")
    parser.add_argument("--rlabel", type=str, default=None, help="Explicit label for the derived third column (e.g. 'IPC', 'dcache HR'). Gets ' [%%]' appended with --rpct. Default: '<num>/<den>'")
    parser.add_argument("--save_png", action='store_true', default=False, help="Save plot as PNG")
    parser.add_argument("--save_svg", action='store_true', default=False, help="Save plot as SVG")
    parser.add_argument("--save_csv", action="store_true", help="Save the complete (unfiltered) flat profile as CSV")
    args = parser.parse_args()

    n = len(args.trace)
    if n not in (1, 2):
        parser.error("-t takes one or two traces")
    if abs(args.sort) not in (1, 2, 3):
        parser.error("--sort must be 1, 2 or 3 (negative for ascending)")
    if args.event is not None and len(args.event) != n:
        parser.error(
            f"-e expects {n} label(s) to match -t, got {len(args.event)}")
    for p in args.trace:
        if not os.path.isfile(p):
            parser.error(f"Trace callstack not found: {p}")
    labels = args.event if args.event is not None else \
        labels_from_paths(args.trace)
    combined = (n == 2)

    print("Running for:")
    for p in args.trace:
        print(f"{INDENT}{p}")
    print()

    if combined:
        fig, ax = plt.subplots(
            1, 3, figsize=(FIG_W_3, FIG_H), tight_layout=True, sharey=True)
    else:
        fig, ax = plt.subplots(
            1, 1, figsize=(FIG_W_1, FIG_H), tight_layout=True)

    fmt = EngFormatter(unit='', places=1, sep="")

    if not combined:
        la = labels[0]
        out_base = os.path.splitext(args.trace[0])[0]
        t_df = run_trace(open(args.trace[0]).readlines(), clk=args.clk)
        title = f"Profile - {la}"

        if args.save_csv:
            save_flat_csv(t_df, out_base + ".csv", labels)

        df_len_og = t_df.index.size
        # filter (significance) before head (top-N) so --top returns N kept rows
        if args.thr is not None:
            t_df = t_df[t_df['percent'] >= args.thr]
        if args.top is not None:
            t_df = t_df.head(args.top)
        df_len = t_df.index.size

        print(title)
        print_profile(t_df)
        for atn, atv in t_df.attrs.items():
            print(atn, ":", atv)

        samples_str = f"{fmt(t_df.attrs['total_samples'])} samples ({la})"
        draw_single_plot(t_df, ax, {
            'data': 'self_counts' if args.plot_abs else 'percent',
            'xlabel': f"{la} samples" + ("" if args.plot_abs else " [%]"),
            'title': title,
            'use_ylabel': True,
            'fmt': fmt if args.plot_abs else '%.1f%%',
            'eng_fmt': args.plot_abs
        })

    else:
        la, lb = labels
        out_base = os.path.splitext(args.trace[0])[0]
        if out_base.endswith(f"_{la}"):
            out_base = out_base[:-len(f"_{la}")]
        out_base += "_combined"

        t_df = run_trace(open(args.trace[0]).readlines(), clk=None)
        s_df = run_trace(open(args.trace[1]).readlines(), clk=None)

        # merge on symbol; a = first trace, b = second
        merged = pd.merge(
            t_df, s_df, on='symbol', how='outer', suffixes=('_a', '_b'))
        # independent host perf streams (e.g. cycles vs instructions) can sample
        # slightly different symbol sets -> outer join leaves NaN counts;
        # symbol seen in one stream but not the other is genuinely 0 in other
        count_cols = [
            'self_counts_a', 'total_counts_a',
            'self_counts_b', 'total_counts_b'
        ]
        merged[count_cols] = merged[count_cols].fillna(0).astype('int64')

        # derived third column: ratio = num/den (den>0), optionally x100 (rpct)
        num, den = ('a', 'b') if args.rinvert else ('b', 'a')

        def ratio_of(nv, dv):
            # num/den, optionally complemented (1-x, e.g. miss rate -> hit rate)
            # before scaling;
            # NaN when the denominator has no samples
            if dv <= 0:
                return float('nan')
            v = (nv / dv)
            if args.rcomplement:
                v = (1.0 - v)
            scale = 100.0 if args.rpct else 1.0
            return round(v * scale, 3)

        merged['ratio'] = merged.apply(
            lambda r: ratio_of(r[f'self_counts_{num}'],r[f'self_counts_{den}']),
            axis=1
        )

        # rank by |sort| column, sign = direction (neg = ascending); NaN sinks
        sort_col = {1: 'percent_a', 2: 'percent_b', 3: 'ratio'}[abs(args.sort)]
        merged = merged.sort_values(
            by=[sort_col, 'symbol'], ascending=[args.sort < 0, True],
            kind='mergesort', na_position='last')

        tot_a = t_df.attrs['total_samples']
        tot_b = s_df.attrs['total_samples']
        merged.attrs[f"total_{la}"] = tot_a
        merged.attrs[f"total_{lb}"] = tot_b
        num_tot, den_tot = (tot_a, tot_b) if args.rinvert else (tot_b, tot_a)
        merged.attrs["ratio_overall"] = ratio_of(num_tot, den_tot)

        # third-column label
        if args.rlabel:
            rlabel = args.rlabel
        else:
            numl, denl = (la, lb) if args.rinvert else (lb, la)
            rlabel = f"1-({numl}/{denl})" if args.rcomplement \
                else f"{numl}/{denl}"

        rlabel = rlabel + (" [%]" if args.rpct else "")
        samples_str = f"{fmt(tot_a)} {la}, {fmt(tot_b)} {lb}"

        if args.save_csv:
            save_flat_csv(merged, out_base + ".csv", labels, rlabel)

        df_len_og = merged.index.size
        # filter before head (top-N) so --top returns N kept rows,
        # even when --sort ascending puts low-count rows first
        if args.thr is not None:
            # thr gate, independent of sort:
            # keep a symbol if it clears the threshold in *either* trace
            sig = merged[['percent_a', 'percent_b']].max(axis=1)
            merged = merged[sig >= args.thr]
        if args.top is not None:
            merged = merged.head(args.top)
        df_len = merged.index.size

        print(f"Profile - combined ({la} vs {lb})")
        print_profile(
            merged, merged=True, labels=labels, rlabel=rlabel)
        for atn, atv in merged.attrs.items():
            print(atn.replace("_", " "), ":", atv)

        draw_single_plot(merged, ax[0], {
            'data': 'self_counts_a' if args.plot_abs else 'percent_a',
            'xlabel': f"{la} samples" + ("" if args.plot_abs else " [%]"),
            'title': f"Profile - {la}",
            'use_ylabel': True,
            'fmt': fmt if args.plot_abs else '%.1f%%',
            'eng_fmt': args.plot_abs
        })
        draw_single_plot(merged, ax[1], {
            'data': 'self_counts_b' if args.plot_abs else 'percent_b',
            'xlabel': f"{lb} samples" + ("" if args.plot_abs else " [%]"),
            'title': f"Profile - {lb}",
            'fmt': fmt if args.plot_abs else '%.1f%%',
            'eng_fmt': args.plot_abs
        })
        draw_single_plot(merged, ax[2], {
            'data': 'ratio',
            'xlabel': rlabel,
            'title': rlabel,
            'fmt': '%.3f',
            'eng_fmt': False
        })

    # resize only height, keep width
    w, h = fig.get_size_inches()
    fig.set_size_inches(w, max(FIG_H, df_len * 0.33))
    ax_l = ax if combined else [ax]
    for i, a in enumerate(ax_l):
        a.margins(y=0.02)
        if combined:
            axr = a.twinx()
            axr.set_yticks(ax[0].get_yticks())
            axr.set_ylim(ax[0].get_ylim())
            axr.set_yticklabels("")
            if i == 2:
                # last one to add labels
                axr.set_yticklabels(ax[0].get_yticklabels())

    test_name = \
        os.path.basename(os.path.dirname(args.trace[0])).replace('out_', '')
    fig.suptitle(f"Summary for {test_name} - {samples_str}", fontsize=13)

    if df_len < df_len_og:
        print(
            f"\n(Showing top {df_len} of {df_len_og} entries after filtering" +\
            f" - Threshold: {args.thr}%" + \
            (f", Top: {args.top}" if args.top is not None else "") + ")")

    if args.save_png:
        fig.savefig(out_base + ".png", dpi=300, bbox_inches="tight")
        print_file_saved("PNG chart", out_base + ".png")

    if args.save_svg:
        fig.savefig(out_base + ".svg", bbox_inches="tight")
        print_file_saved("SVG chart", out_base + ".svg")

    if args.plot:
        plt.show()
    plt.close('all')

if __name__ == "__main__":
    main()
