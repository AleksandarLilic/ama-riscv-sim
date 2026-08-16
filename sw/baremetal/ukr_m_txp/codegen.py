#!/usr/bin/env python3

"""Codegen for the block transpose test: c(N x M) = a(M x N)^T

Emits one self-contained '#if defined(NF_*)' block per type:
the source operand, the poisoned destination, and the reference

"""

import argparse
import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import codegen_txp as cg
from codegen_common import *

OUT = "test_arrays.h"

parser = argparse.ArgumentParser()
cg.add_dim_args(parser)
parser.add_argument("--types", nargs="+", required=True, help="NF tokens, passed straight from the Makefile's TYPES")
args = parser.parse_args()

M, N = args.M, args.N
TYPES = [nkey(t) for t in args.types]

# the block is elements per word and is forced, not tuned, so every type in the
# sweep constrains the shape; all are powers of 2, so max == lcm
BLK = max(el_per_word(t) for t in TYPES)
if (M % BLK) or (N % BLK):
    sys.exit(f"M {M} and N {N} both have to be multiples of {BLK}")

code = []
code.append("#pragma once\n")
code.append("#include <stdint.h>\n")
code.append(f"#define M {M}")
code.append(f"#define N {N}\n")

for i, t in enumerate(TYPES):
    d = cg.gen(
        M, N, t, lda=args.lda, ldc=args.ldc,
        max_bytes=args.max_bytes, seed_in=args.seed
    )
    cg.self_check(d)

    code.append(
        ("#if " if not i else "#elif ") + f"defined(NF_{NUM[t]['macro']})"
    )
    # both strides are per type: the round-up follows elements per word
    code.append(f"#define LDA {d.lda}")
    code.append(f"#define LDC {d.ldc}")

    code.append(emit_panel(
        "a", d.a, t, dim_el="M*LDA", dim_2d=["M", "LDA"], ld=d.lda,
        align=ALIGN
    ))
    # comment meaningless, c poisoned
    code.append(emit_panel(
        "c", d.c, t, dim_el="N*LDC", dim_2d=["N", "LDC"],
        ld=d.ldc, align=ALIGN, add_comment=False
    ))
    # ref carries the padding too, so the compare covers it
    code.append(emit_panel(
        "ref", d.ref, t, dim_el="N*LDC", dim_2d=["N", "LDC"],
        ld=d.ldc, align=ALIGN, qual="const "
    ) + "\n")

    print(
        f"{t:9s} a={n_bytes(M * d.lda, t):5d}B c={n_bytes(N * d.ldc, t):5d}B "
        f"lda={d.lda:4d} ldc={d.ldc:4d} "
        f"blocks={(M // el_per_word(t)) * (N // el_per_word(t)):4d}"
    )

finish_gen(code, OUT)
