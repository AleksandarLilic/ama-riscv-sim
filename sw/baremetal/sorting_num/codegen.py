import os
import sys
import random
import numpy as np

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from codegen_common import *

LEN_MAP = {"tiny": 10, "small": 30, "medium": 100, "large": 400}
LEN_NAMES = list(LEN_MAP.keys())

if len(sys.argv) != 2:
    print("Usage: python3 codegen.py <" + "|".join(LEN_NAMES) + ">")
    sys.exit(1)

len_name = sys.argv[1]
if len_name not in LEN_NAMES:
    print(f"ARR_LEN {len_name} is not in {LEN_NAMES}")
    sys.exit(1)

arr_len = LEN_MAP[len_name]
OUT = f"test_arrays_{len_name}.h"

code = []
code.append("#include <stdint.h>\n")
code.append(f"#define ARR_LEN {arr_len}\n")

random.seed(0)
for key,value in NUM.items():
    def_check = "#if " if key == "uint8_t" else "#elif "
    code.append(def_check + "defined(NF_" + \
                value["nf"].__name__.upper() + ")")

    if "float" in value["nf"].__name__:
        typ_min = value["min"]
        typ_max = value["max"]
        ctypes = FP_C_MAP[value["nf"]]
    else:
        typ_min = np.iinfo(value["nf"]).min
        typ_max = np.iinfo(value["nf"]).max
        ctypes = value["nf"].__name__ + "_t"

    code.append("#define NF_IN " + ctypes)

    value['a'] = rnd_gen_1d_arr(typ_min, typ_max, arr_len, value["nf"])
    value['ref'] = np.sort(value['a'])

    suffix = "ULL" if key == "uint64_t" else ""
    code.append(np2c_1d_arr('a', value['a'], "NF_IN", suffix))
    code.append(np2c_1d_arr('ref', value['ref'], "NF_IN", suffix) + "\n")

finish_gen(code, OUT)
