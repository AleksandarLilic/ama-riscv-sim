import os
import sys
import random
import numpy as np

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from codegen_common import *

ARR_LEN = 64
OUT = f"test_arrays.h"

code = []
code.append("#include <stdint.h>\n")
code.append(f"#define ARR_LEN {ARR_LEN}\n")

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

    value['a'] = rnd_gen(typ_min, typ_max, ARR_LEN, value["nf_in"])
    value['ref'] = np.sort(value['a'])

    suffix = "ULL" if key == "uint64_t" else ""
    code.append(np2c('a', value['a'], "in", suffix))
    code.append(np2c('ref', value['ref'], "in", suffix) + "\n")

finish_gen(code, OUT)
