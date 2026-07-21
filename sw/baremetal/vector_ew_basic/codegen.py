import os
import sys
import random
import numpy as np

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from codegen_common import *

def add(a, b): return a + b
def sub(a, b): return a - b
def mul(a, b): return a * b
def div(a, b): return a / b

ARR_LEN = 128
OPS = [add, sub, mul, div]
OPS_N = [op.__name__ for op in OPS]
OPS_SIGN = ["+", "-", "*", "/"]

if len(sys.argv) != 2:
    print(f"Usage: python3 codegen.py <{'|'.join(OPS_N)}>")
    sys.exit(1)

op = sys.argv[1]
if op not in OPS_N:
    print(f"Operation {op} not found in {OPS_N}")
    sys.exit(1)

OUT = f"test_arrays_{op}.h"

# find the operation
op, idx = [zop for zop in zip(OPS, range(len(OPS))) if zop[0].__name__ == op][0]

code = []
code.append("#include <stdint.h>\n")
code.append(f"#define ARR_LEN {ARR_LEN}\n")
code.append(f"#define OP {OPS_SIGN[idx]}\n")

random.seed(0)
for idx, (key, value) in enumerate(iter_num("int", "uint", "fp", narrow=False)):
    ifdef = "#if " if not idx else "#elif "
    code.append(ifdef + "defined(NF_" + value["macro"] + ")")
    code.append("#define NF " + value["ctype"])
    op_name = op.__name__

    if value["kind"] == "fp":
        typ_min, typ_max = value["min"], value["max"]
    else:
        shift_amount = value.get("offset", {"offset": {}}).get(op_name, 0)
        typ_min = value["min"] >> shift_amount
        typ_max = value["max"] >> shift_amount


    value['a'] = rnd_gen_1d_arr(typ_min, typ_max, ARR_LEN, value["nf"])
    value['b'] = rnd_gen_1d_arr(typ_min, typ_max, ARR_LEN, value["nf"])

    # bias for unsigned int subtraction
    if "uint" in key and op_name == "sub":
        value['b'] = value['b'] >> (2 + ("32" in key)*2 + ("64" in key)*4)
        value['b'] = np.minimum(value['a'], value['b'])

    # bias for int division
    if "int" in key and op_name == "div":
        value['b'] = value['b'] >> (4 + ("32" in key)*2 + ("64" in key)*6)
        # ensure no zeros in the denominator
        value['b'] = np.where(value['b'] == 0, 13, value['b'])

    # ensure no zeros in the denominator for float division
    if "float" in key and op_name == "div":
        value['b'] = np.where(value['b'] == 0, 0.01, value['b'])

    nf_out, ctype_out = value["nf"], "NF" # NF matches defined type
    # allow for double width output types for mul of 8 and 16 bit
    if op_name == "mul":
        if key == "uint8_t":
            nf_out = np.uint16
            ctype_out = "uint16_t"
        elif key == "int8_t":
            nf_out = np.int16
            ctype_out = "int16_t"
        elif key == "uint16_t":
            nf_out = np.uint32
            ctype_out = "uint32_t"
        elif key == "int16_t":
            nf_out = np.int32
            ctype_out = "int32_t"

    # cast all to nf
    value['a'] = value['a'].astype(nf_out)
    value['b'] = value['b'].astype(nf_out)
    value['ref'] = np.zeros(ARR_LEN, dtype=nf_out)

    # calculate the result
    for i in range(ARR_LEN):
        value['ref'][i] = op(value['a'][i], value['b'][i])

    code.append(np2c_1d_arr('a', value['a']))
    code.append(np2c_1d_arr('b', value['b']))
    code.append(np2c_1d_arr('c', [0], nf=ctype_out))
    code.append(np2c_1d_arr('ref', value['ref'], nf=ctype_out) + "\n")

finish_gen(code, OUT)
