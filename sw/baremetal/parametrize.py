#!/usr/bin/env python3

"""
Generate a Makefile fragment (TARGETS + per-target EXTRA_DEFINES)
for parametrized multi-binary tests.

Usage:
    parametrize.py [--groups] "values:PREFIX" ["values:PREFIX" ...]

Each positional argument describes one axis:
  values  space-separated list of values for that axis
  PREFIX  define prefix:
            WORD  ->  -DWORD_VALUE  (e.g. NF -> -DNF_INT8)
            ""    ->  -DVALUE       (e.g. "" -> -DLOAD)
            -     ->  no define     (axis contributes to target name only)

Without --groups, outputs:
    TARGETS = a_x.elf a_y.elf ...
    a_x.elf: EXTRA_DEFINES := -DA_... -DX_...

With --groups, the first axis values become variable name suffixes:
    TARGETS_A = a_x.elf a_y.elf ...
    a_x.elf: EXTRA_DEFINES := -DA_... -DX_...
    TARGETS_B = b_x.elf b_y.elf ...
    ...

Examples:
    parametrize.py "int8:-" "ijk jik jki:LOOP_ORDER"
    parametrize.py --groups "add sub mul div:OP" "uint8 int8 int16:NF"
"""

import argparse
import itertools


def parse_axis(arg):
    """'val1 val2:PREFIX' -> ([val1, val2], PREFIX)"""
    vals, _, prefix = arg.rpartition(':')
    return vals.split(), prefix


def make_flag(value, prefix):
    if prefix == '-':
        return None
    return f'-D{prefix}_{value.upper()}' if prefix else f'-D{value.upper()}'


def emit(axes, var_name):
    combos = list(itertools.product(*[vals for vals, _ in axes]))
    targets = ['_'.join(combo) + '.elf' for combo in combos]
    print(f'{var_name} =', ' '.join(targets))
    for combo, target in zip(combos, targets):
        flags = []
        for val, (_, pfx) in zip(combo, axes):
            f = make_flag(val, pfx)
            if f:
                flags.append(f)
        if flags:
            print(f'{target}: EXTRA_DEFINES :=', ' '.join(flags))


parser = argparse.ArgumentParser()
parser.add_argument('--groups', action='store_true',
                    help='first axis values become TARGETS_GROUP variable names')
parser.add_argument('axes', nargs='+')
args = parser.parse_args()

axes = [parse_axis(a) for a in args.axes]

if args.groups:
    group_vals, group_pfx = axes[0]
    rest = axes[1:]
    for g in group_vals:
        emit([([g], group_pfx)] + rest, f'TARGETS_{g.upper()}')
else:
    emit(axes, 'TARGETS')
