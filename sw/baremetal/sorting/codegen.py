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
                value["nf_in"].__name__.upper() + ")")

    if "float" in value["nf_in"].__name__:
        typ_min = value["min"]
        typ_max = value["max"]
        ctypes = FP_C_MAP[value["nf_in"]]
    else:
        typ_min = np.iinfo(value["nf_in"]).min
        typ_max = np.iinfo(value["nf_in"]).max
        ctypes = value["nf_in"].__name__ + "_t"

    code.append("#define NF_IN " + ctypes)

    value['a'] = rnd_gen(typ_min, typ_max, arr_len, value["nf_in"])
    value['ref'] = np.sort(value['a'])

    suffix = "ULL" if key == "uint64_t" else ""
    code.append(np2c_arr('a', value['a'], "in", suffix))
    code.append(np2c_arr('ref', value['ref'], "in", suffix) + "\n")

finish_gen(code, OUT)
