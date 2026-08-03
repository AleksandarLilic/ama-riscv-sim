#!/usr/bin/env python3

import os
import random
import sys

import numpy as np

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from codegen_common import *

if len(sys.argv) != 5:
    print(f"Usage: codegen.py <nf> <M> <N> <K>")
    sys.exit(1)

NF_IN = str(sys.argv[1])
NF = f"{NF_IN}_t" if 'int' in NF_IN else NF_IN
if NF not in NUM.keys():
    print(f"Unsupported number format: '{NF}'")
    sys.exit(1)
M = int(sys.argv[2])
N = int(sys.argv[3])
K = int(sys.argv[4])
OUT = f"test_matrices_{NF_IN}.h"

code = []
code.append("#pragma once\n")
code.append("#include <stdint.h>\n")
code.append(f"#define M {M}")
code.append(f"#define N {N}")
code.append(f"#define K {K}\n")

random.seed(0)
value = NUM[NF]
shift_amount = 0
typ_min = value["min"] >> shift_amount
typ_max = value["max"] >> shift_amount

value['a'] = rnd_gen_2d_arr(typ_min, typ_max, M, K, value["nf"])
value['b'] = rnd_gen_2d_arr(typ_min, typ_max, K, N, value["nf"])
ref = np.matmul(value['a'].astype(np.int32), value['b'].astype(np.int32))

#print(value['a'])
#print(value['b'])
#print(ref)
#print(np2c_2d_arr('a', value['a'], "int8_t"))

code.append(np2c_2d_arr('a', value['a'], NF, ["M", "K"]) + "\n")
code.append(np2c_2d_arr('b', value['b'], NF, ["K", "N"]) + "\n")
code.append(np2c_2d_arr('c', np.zeros_like(ref), "int32_t", ["M", "N"]) + "\n")
code.append(np2c_2d_arr('ref', ref, "int32_t", ["M", "N"]) + "\n")

finish_gen(code, OUT, add_assert=False)
