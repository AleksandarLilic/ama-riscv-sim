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

ARR_LEN = 32
OPS = [add, sub, mul, div]
OPS_N = [op.__name__ for op in OPS]
OPS_SIGN = ["+", "-", "*", "/"]

op = sys.argv[1]
if op not in OPS_N:
    print(f"Operation {op} not found in {OPS_N}")
    sys.exit(1)

OUT = f"test_arrays_{op}.h"

# find the operation
op, idx = [zop for zop in zip(OPS, range(len(OPS)))
           if zop[0].__name__ == op][0]

code = []
code.append("#include <stdint.h>\n")
code.append(f"#define ARR_LEN {ARR_LEN}\n")
code.append("#define OP " + OPS_SIGN[idx] + "\n")

random.seed(0)
for key,value in NUM.items():
    def_check = "#if " if key == "uint8_t" else "#elif "
    code.append(def_check + "defined(NF_" + \
                value["nf_in"].__name__.upper() + ")")

    if "float" in value["nf_in"].__name__:
        typ_min = value["min"]
        typ_max = value["max"]
        ctypes = [FP_C_MAP[value["nf_in"]], FP_C_MAP[value["nf_out"]]]
    else:
        shift_amount = value.get(f"off_{op.__name__}", 0)
        typ_min = np.iinfo(value["nf_in"]).min >> shift_amount
        typ_max = np.iinfo(value["nf_in"]).max >> shift_amount
        ctypes = [value["nf_in"].__name__ + "_t",
                  value["nf_out"].__name__ + "_t"]

    code.append("#define NF_IN " + ctypes[0])
    code.append("#define NF_OUT " + ctypes[1])

    value['a'] = rnd_gen(typ_min, typ_max, ARR_LEN, value["nf_in"])
    value['b'] = rnd_gen(typ_min, typ_max, ARR_LEN, value["nf_in"])

    # bias for unsigned int subtraction
    if "uint" in key and op.__name__ == "sub":
        value['b'] = value['b'] >> (2 + ("32" in key)*2 + ("64" in key)*4)
        value['b'] = np.minimum(value['a'], value['b'])

    # bias for int division
    if "int" in key and op.__name__ == "div":
        value['b'] = value['b'] >> (4 + ("32" in key)*2 + ("64" in key)*6)
        # ensure no zeros in the denominator
        value['b'] = np.where(value['b'] == 0, 13, value['b'])

    # ensure no zeros in the denominator for float division
    if "float" in key and op.__name__ == "div":
        value['b'] = np.where(value['b'] == 0, 0.01, value['b'])

    # cast all to nf_out
    value['a'] = value['a'].astype(value["nf_out"])
    value['b'] = value['b'].astype(value["nf_out"])
    value['ref'] = np.zeros(ARR_LEN, dtype=value["nf_out"])

    # calculate the result
    for i in range(ARR_LEN):
        value['ref'][i] = op(value['a'][i], value['b'][i])

    code.append(np2c('a', value['a'], "in"))
    code.append(np2c('b', value['b'], "in"))
    code.append(np2c('c', [0], "out"))
    code.append(np2c('ref', value['ref'], "out") + "\n")

finish_gen(code, OUT)
