#!/usr/bin/env python3

import os
import random
import sys

import numpy as np

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from codegen_common import *

TYPES = ["int16", "int8", "int4", "int2"] # a/b operand types, widest first
ERAN = {"int16": 1, "int8": 1, "int4": 2, "int2": 4} # elements per byte boundry
COMBS = [(x, y) for i, x in enumerate(TYPES) for y in TYPES[i:]]

MAX_BYTES = 16*1024 # per vector
MAX_ATTEMPTS = 1000 # re-draw cap, realy only int16*int16 can overflow int32
INT32_MIN, INT32_MAX = -2**31, 2**31-1
OUT = "test_arrays.h"

if len(sys.argv) != 2:
    print(f"Usage: codegen.py <ARR_LEN>")
    sys.exit(1)

ARR_LEN = int(sys.argv[1])
# multiple of 16: no scalar tail on the aligned path for any type
# at least 32: below that the unaligned int2 case has no simd iterations
if ARR_LEN % 16 or ARR_LEN < 32:
    print(f"ARR_LEN {ARR_LEN} has to be a multiple of 16, and at least 32")
    sys.exit(1)

nbytes = {
    t: (ARR_LEN * 2 if t == "int16" else ARR_LEN // ERAN[t]) for t in TYPES
}
if max(nbytes.values()) > MAX_BYTES:
    print(f"ARR_LEN {ARR_LEN} exceeds {MAX_BYTES}B per vector: {nbytes}")
    sys.exit(1)
print(f"Data: {2*sum(nbytes.values())}B total, per vector: {nbytes}")

def as_name(x, y):
    return x if x == y else f"{x}_{y}"

def dotp(x, y, l):
    return int(np.dot(x[:l].astype(np.int64), y[:l].astype(np.int64)))

def align(c):
    # align to the boundary of the smallest element (largest el range)
    emax = max(ERAN[c[0]], ERAN[c[1]])
    return ((ARR_LEN-1) // emax) * emax

# unaligned length: back off one element, then down to the byte boundary
LEN_UAL = {c: align(c) for c in COMBS}

for attempt in range(MAX_ATTEMPTS):
    random.seed(attempt)
    lval, pval = {}, {} # logical values (drive the refs), and packed values
    for t in TYPES:
        value = NUM[f"{t}_t"]
        for v in ("a", "b"):
            # (operand, type) pair as key
            if "narrow_bits" in value:
                lval[v, t], pval[v, t] = rnd_gen_1d_arr_narrow(value, ARR_LEN)
            else:
                lval[v, t] = pval[v, t] = rnd_gen_1d_arr(
                    value["min"], value["max"], ARR_LEN, value["nf"]
                )

    refs = { # (aligned, unaligned) pair
        c: (dotp(lval["a", c[0]], lval["b", c[1]], ARR_LEN),
            dotp(lval["a", c[0]], lval["b", c[1]], LEN_UAL[c]))
            for c in COMBS
        }
    bad = [c for c, r in refs.items()
           if not all(INT32_MIN <= x <= INT32_MAX for x in r)]
    if not bad:
        break
else:
    raise RuntimeError(
        f"no int32-safe draw for {[as_name(*c) for c in bad]} "
        f"at ARR_LEN {ARR_LEN} in {MAX_ATTEMPTS} attempts"
    )

code = []
code.append("#pragma once\n")
code.append("#include <stdint.h>\n")
code.append(f"#define ARR_LEN {ARR_LEN}\n")

# one array per type per operand, shared by every combo that selects it
for v, pos in (("a", 0), ("b", 1)):
    for i, t in enumerate(TYPES):
        value = NUM[f"{t}_t"]
        guard = " || ".join(
            f"defined(NF_{as_name(*c).upper()})" for c in COMBS if c[pos] == t
        )
        code.append(("#if " if not i else "#elif ") + guard)
        if "narrow_bits" in value:
            code.append(np2c_1d_arr_narrow(
                v, lval[v, t], pval[v, t], nf=value["ctype"])
            )
        else:
            code.append(np2c_1d_arr(v, pval[v, t], nf=value["ctype"]))
    code.append("#else")
    code.append(f'_Static_assert(0, "Unsupported number format: {v}");')
    code.append("#endif\n")

# vector length and reference, per combo, for both aligned and unaligned builds
for i, c in enumerate(COMBS):
    ref_al, ref_ual = refs[c]
    code.append(
        ("#if " if not i else "#elif ") + f"defined(NF_{as_name(*c).upper()})"
    )
    code.append("#ifdef UAL")
    code.append(f"#define VEC_LEN {LEN_UAL[c]}")
    code.append(f"const int32_t ref = {ref_ual};")
    code.append("#else")
    code.append("#define VEC_LEN ARR_LEN")
    code.append(f"const int32_t ref = {ref_al};")
    code.append("#endif\n")

finish_gen(code, OUT)
