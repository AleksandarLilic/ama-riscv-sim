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
for idx, (key, value) in enumerate(iter_num("int", "uint", "fp", narrow=False)):
    ifdef = "#if " if not idx else "#elif "
    code.append(ifdef + "defined(NF_" + value["macro"] + ")")
    code.append("#define NF_IN " + value["ctype"])

    value['a'] = rnd_gen_1d_arr(
        value["min"], value["max"], arr_len, value["nf"]
    )
    value['ref'] = np.sort(value['a'])

    suffix = "ULL" if key == "uint64_t" else ""
    code.append(np2c_1d_arr('a', value['a'], "NF_IN", suffix))
    code.append(np2c_1d_arr('ref', value['ref'], "NF_IN", suffix) + "\n")

finish_gen(code, OUT)
