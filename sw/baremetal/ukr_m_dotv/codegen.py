#!/usr/bin/env python3

"""Shared codegen for the A(M x K) @ B(K x N) test family.

Symlinked into each test dir; the Makefile supplies the shape via CLI

Emits one self-contained '#if defined(NF_*)' block per comb:
the operands, the row stride, the reduction length, and the reference

--b_t and --c_t do the same thing to their respective matrices:
flip which axis is contiguous in memory; neither touches the math
"""

import argparse
import os
import sys

import numpy as np

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import codegen_gemm as cg
from codegen_common import *

OUT = "test_arrays.h"
ALIGN = 4 # word loads on the SIMD/LOAD_OPT paths, -mstrict-align

def nf_comb(tok):
    """'int8' -> (int8, int8); 'int16_int8' -> (int16, int8).
    Tokens are the Makefile's TYPES verbatim, so they double as the NF_* name"""
    parts = tok.split("_")
    if len(parts) == 1:
        return (parts[0], parts[0])
    if len(parts) == 2:
        return (parts[0], parts[1])
    raise argparse.ArgumentTypeError(f"{tok}: expected 'ta' or 'ta_tb'")

parser = argparse.ArgumentParser()
cg.add_dim_args(parser)
parser.add_argument("--types", nargs="+", required=True, type=nf_comb, help="NF tokens, passed straight from the Makefile's TYPES")
parser.add_argument("--k_step", type=int, default=1, help="widest kernel K tile in elements (SIMD_UNROLL for dotv, K_STEP for dotf); K must be a multiple of it so the aligned target has no tail")
parser.add_argument("--skip_k_limit_check", action="store_true", help="don't enforce K-limit check")
parser.add_argument("--ual", action="store_true", help="also emit the unaligned companion length and its reference")
parser.add_argument("--no_flatten", action="store_true", help="emit 2D arrays instead of flat pointer+stride ones")
parser.add_argument("--b_t", action="store_true", help="store B as N rows of k-contiguous data instead of K rows of n-contiguous data, for a kernel that walks both operands along k; LDB then strides k and B's padding moves with it")
parser.add_argument("--c_t", action="store_true", help="lay the outputs out as (N, M) instead of (M, N), for a kernel that writes C batch-major; layout only, the values are the same")
parser.add_argument("--out_buf", nargs=2, default=None, metavar=("NAME", "INIT"), help="int32 destination buffer to emit, e.g. 'y poison'. INIT is 'poison' when the kernel owns every element (write-only, so a skipped or accumulating store is caught), or 'zero' when the kernel accumulates into it")
args = parser.parse_args()

OUT_INIT = {
    "poison": lambda n: poison_arr(n, np.int32),
    "zero": lambda n: np.zeros(n, dtype=np.int32),
}
if args.out_buf and (args.out_buf[1] not in OUT_INIT):
    sys.exit(
        f"--out_buf INIT is one of {sorted(OUT_INIT)}, got '{args.out_buf[1]}'"
    )

M, N, K = args.M, args.N, args.K
COMBS = args.types

# K has to clear both the element packing tile and the kernel's own tile,
# so that the aligned target has no scalar tail;
# all are powers of 2, so max == lcm
EL_TILE = max(el_per_word(nkey(t)) for c in COMBS for t in c)
K_TILE = max(EL_TILE, args.k_step)
if K % K_TILE:
    sys.exit(f"K {K} has to be a multiple of {K_TILE}")

# 2x: the unaligned flavor still gets at least one full tile for simd
if not args.skip_k_limit_check and (K < (2 * K_TILE)):
    sys.exit(
        f"K {K} has to be at least {2 * K_TILE} "
        f"(element tile {EL_TILE}, kernel k_step {args.k_step})"
    )

# at N == 1 'B' is a contiguous K-vector with ldb 1,
# so a 2D 'b' would be K rows of a single el, and a zero-width row once packed
if args.no_flatten and (N == 1):
    sys.exit("--no_flatten needs N > 1, 'b' is a plain K-vector at N == 1")

def emit_operand(name, packed, logical, t, dim_el, dim_2d, ld):
    """flat 'pointer + stride' by default, 2D with --no_flatten\n
    sub-byte types emitted packed, logical lanes kept alongside as a comment"""
    value = NUM[t]
    is_narrow = "narrow_bits" in value
    # C entries per row, which is the storage count, not the logical one;
    # <= 1 means there is no row worth breaking on, e.g. 'b' as a contiguous
    # K-vector at N == 1, where ldb is 1
    row_len = ld // el_per_byte(t)
    row_len = row_len if (row_len > 1) else None

    dim_2d_packed = [dim_2d[0], storage_dim(dim_2d[1], t)]
    if args.no_flatten:
        if not is_narrow:
            return np2c_2d_arr(
                name, logical, nf=value["ctype"], dim=dim_2d_packed, align=ALIGN
            )
        # a packed row is always a whole number of bytes, so the flat packing
        # reshapes into rows as is - no lane straddles a row boundary
        rows, per_row = logical.shape[0], (ld // el_per_byte(t))
        return np2c_2d_arr_narrow(
            name, logical, packed.reshape(rows, per_row), nf=value["ctype"],
            dim=dim_2d, dim_packed=dim_2d_packed, align=ALIGN
        )

    if not is_narrow:
        return np2c_1d_arr(
            name, packed, nf=value["ctype"], dim=storage_dim(dim_el, t),
            align=ALIGN, row_len=row_len
        )
    return np2c_1d_arr_narrow(
        name, logical.reshape(-1), packed, nf=value["ctype"], dim=dim_el,
        dim_packed=storage_dim(dim_el, t), align=ALIGN, row_len=row_len
    )

def emit_out(name, arr, nf, align=None):
    """the M x N outputs: reference, or the poisoned destination buffer\n
    --c_t transposes the layout, --no_flatten follows the operands"""
    arr = np.asarray(arr).reshape(M, N)
    arr = arr.T if args.c_t else arr
    dim = ["N", "M"] if args.c_t else ["M", "N"]
    if args.no_flatten:
        return np2c_2d_arr(name, arr, nf=nf, dim=dim, align=align)
    return np2c_1d_arr(
        name, [int(x) for x in arr.reshape(-1)], nf=nf,
        dim=f"{dim[0]}*{dim[1]}", align=align,
        # one row per line, so a single column stays on one line
        row_len=(arr.shape[1] if arr.shape[1] > 1 else None)
    )

def emit_ref(d, k):
    return emit_out(
        "ref", (d.ref if k == K else d.ref_at(k)), "const int32_t"
    )

code = []
code.append("#pragma once\n")
code.append("#include <stdint.h>\n")
code.append(f"#define M {M}")
code.append(f"#define N {N}")
code.append(f"#define K {K}")
# the outputs' inner dim, so a kernel taking an 'ldc' can be handed it; unpadded
code.append(f"#define LDC {'M' if args.c_t else 'N'}\n")

for i, (ta, tb) in enumerate(COMBS):
    d = cg.gen(
        M, N, K, ta, tb, lda=args.lda, ldb=args.ldb, b_t=args.b_t,
        max_bytes=args.max_bytes, seed_in=args.seed,
        overflow_check=(not args.no_overflow_check)
    )
    cg.self_check(d)

    nf = ta if ta == tb else f"{ta}_{tb}"
    code.append(("#if " if not i else "#elif ") + f"defined(NF_{nf.upper()})")
    # both strides are per comb: tile size follows the type of each operand
    # LDB strides n, so it is 1 at N == 1 where B is a contiguous K-vector;
    # under --b_t it strides k instead, and B's rows are the N of them
    code.append(f"#define LDA {d.lda}")
    code.append(f"#define LDB {d.ldb}")

    b_dim = ["N", "LDB"] if args.b_t else ["K", "LDB"]
    code.append(emit_operand(
        "a", d.a, d.a_log, d.ta, dim_el="M*LDA", dim_2d=["M", "LDA"], ld=d.lda
    ))
    code.append(emit_operand(
        "b", d.b, d.b_sl, d.tb, dim_el=f"{b_dim[0]}*{b_dim[1]}",
        dim_2d=b_dim, ld=d.ldb
    ))
    if args.out_buf:
        name, init = args.out_buf
        code.append(emit_out(
            name, OUT_INIT[init](M * N), "int32_t", align=ALIGN
        ))

    # reduction length and reference, for the aligned and unaligned builds
    if args.ual:
        code.append("#ifdef UAL")
        code.append(f"#define VEC_LEN {d.k_ual}")
        code.append(emit_ref(d, d.k_ual))
        code.append("#else")
        code.append("#define VEC_LEN K")
        code.append(emit_ref(d, K))
        code.append("#endif\n")
    else:
        code.append("#define VEC_LEN K")
        code.append(emit_ref(d, K) + "\n")

    sa, sb = d.shift
    print(
        f"{nf:12s} a={n_bytes(M * d.lda, d.ta):5d}B "
        f"b={n_bytes(d.b_sl.shape[0] * d.ldb, d.tb):5d}B "
        f"lda={d.lda:4d} ldb={d.ldb:4d} ual={d.k_ual:4d} "
        + ("full range" if not (sa or sb) else f"range >>{sa}/{sb} + inject")
    )

finish_gen(code, OUT)
