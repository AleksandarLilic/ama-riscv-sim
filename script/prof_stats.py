#!/usr/bin/env python3

import sys
import argparse
from typing import Dict, Any, Tuple, List, Iterable, Optional
from collections import defaultdict, OrderedDict

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

def accumulate_samples(lines: Iterable[str]) -> \
Tuple[Dict[str, int], Dict[str, int], int]:
    """
    Given lines of folded stacks, accumulate:
      self_counts[func] = sum of counts where func is leaf
      total_counts[func] = sum of counts where func appears anywhere in the path
      total_samples = sum of all counts
    """
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

    return self_counts, total_counts, total_samples

def format_flat_profile(
    self_counts: Dict[str, int],
    total_counts: Dict[str, int],
    total_samples: int,
    top_n: Optional[int] = None) \
-> List[Tuple[float, float, int, int, str]]:
    """
    Prepare rows sorted by descending self_counts.
    Returns a list of tuples: (percent, cum_percent, self, total, func_name).
    """

    # build list of (func, self, total)
    items = []
    for func, self_c in self_counts.items():
        total_c = total_counts.get(func, 0)
        items.append((func, self_c, total_c))

    # functions that never appear as leaf but appear in total?
    # they have self=0. If desired to include them, uncomment:
    # for func, tot_c in total_counts.items():
    #     if func not in self_counts:
    #         items.append((func, 0, tot_c))

    # sort descending by self count, then by name
    items.sort(key=lambda x: (-x[1], x[0]))
    rows = []
    cum_percent = 0.0
    for func, self_c, total_c in items:
        percent = (self_c / total_samples * 100) if total_samples > 0 else 0.0
        cum_percent += percent
        rows.append((percent, cum_percent, self_c, total_c, func))

    if top_n is not None:
        rows = rows[:top_n]

    return rows

def print_profile(rows: List[Tuple[float, float, int, int, str]]) -> None:
    """
    Print the flat profile lines with columns:
    %   cumulative   self   total   name
    """

    #print("Each sample counts as one sample unit.")
    hdr = "{:>6} {:>10} {:>10} {:>10}   {}" \
          .format("%", "cumulative", "self", "total", "name")
    print(hdr)

    for percent, cum_percent, self_c, total_c, func in rows:
        print("{:6.2f} {:10.2f} {:10d} {:10d}   {}"
              .format(percent, cum_percent, self_c, total_c, func))

def main():
    parser = argparse.ArgumentParser(description="Convert folded callstack samples to a flat-profile summary.")
    parser.add_argument("infile", nargs="?", default="-", help="Input file with folded stacks")
    parser.add_argument( "--top", type=int, default=None, help="Show only the top N functions by self-samples.")
    args = parser.parse_args()

    with open(args.infile, "r") as f:
        lines = f.readlines()

    try:
        self_counts, total_counts, total_samples = accumulate_samples(lines)
    except ValueError as e:
        print(f"Error parsing input: {e}", file=sys.stderr)
        sys.exit(1)

    if total_samples == 0:
        print("No samples found.", file=sys.stderr)
        sys.exit(1)

    rows = format_flat_profile(
        self_counts, total_counts, total_samples, top_n=args.top)
    print_profile(rows)

if __name__ == "__main__":
    main()
