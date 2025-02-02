import os
import sys
import random
import numpy as np

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from codegen_common import *

if len(sys.argv) != 4:
    print(f"Usage: python3 codegen.py <i (A_ROWS)> <j (B_COLS)> <k (A_COLS/B_ROWS)>")
    sys.exit(1)

A_ROWS = int(sys.argv[1])
B_COLS = int(sys.argv[2])
A_COLS_B_ROWS = int(sys.argv[3])
OUT = f"test_matrices.h"

code = []
code.append("#include <stdint.h>\n")
code.append(f"#define A_ROWS {A_ROWS}")
code.append(f"#define B_COLS {B_COLS}")
code.append(f"#define A_COLS_B_ROWS {A_COLS_B_ROWS}\n")

random.seed(0)
value = NUM["int8_t"]
ctype = np2c_type(value["nf"])
shift_amount = 0
typ_min = np.iinfo(value["nf"]).min >> shift_amount
typ_max = np.iinfo(value["nf"]).max >> shift_amount


value['a'] = rnd_gen_2d_arr(
    typ_min, typ_max, A_ROWS, A_COLS_B_ROWS, value["nf"])
value['b'] = rnd_gen_2d_arr(
    typ_min, typ_max, A_COLS_B_ROWS, B_COLS, value["nf"])
ref = np.matmul(value['a'].astype(np.int32), value['b'].astype(np.int32))

#print(value['a'])
#print(value['b'])
#print(ref)
#print(np2c_2d_arr('a', value['a'], "int8_t"))

code.append(np2c_2d_arr('a', value['a'], "int8_t", ["A_ROWS", "A_COLS_B_ROWS"]) + "\n")
code.append(np2c_2d_arr('b', value['b'], "int8_t", ["A_COLS_B_ROWS", "B_COLS"]) + "\n")
code.append(np2c_2d_arr('c', np.zeros_like(ref), "int32_t", ["A_ROWS", "B_COLS"]) + "\n")
code.append(np2c_2d_arr('ref', ref, "int32_t", ["A_ROWS", "B_COLS"]) + "\n")

finish_gen(code, OUT, add_assert=False)
