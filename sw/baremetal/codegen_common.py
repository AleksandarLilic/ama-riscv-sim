import random
import numpy as np

# format numpy array as C array
def np2c_type(nf):
    if nf in FP_C_MAP:
        return FP_C_MAP[nf]
    return nf.__name__ + "_t"

def np2c_1d_arr(var, arr, nf="NF", dim="ARR_LEN", suffix="", str_type=False):
    if str_type:
        arr_out = [f"\"{x}\"" for x in arr]
    else:
        arr_out = [f"{x}" for x in arr]

    return f"{nf} " + var + f"[{dim}]" + " = {" + \
        f"{suffix}, ".join(arr_out) + suffix + "};"

def np2c_2d_arr(var, arr, nf="NF", dim=["A", "B"], suffix=""):
    arr_out = []
    for row in arr:
        arr_out.append("\n    {" + ", ".join([f"{x}" for x in row]) + "}")

    return f"{nf} " + var + f"[{dim[0]}][{dim[1]}]" + " = {" + \
        f"{suffix}, ".join(arr_out) + suffix + "\n};"

NUM = {
    "uint8_t": {"off_add": 2, "off_sub": 2, "off_mul": 1, "nf": np.uint8},
    "int8_t": {"off_add": 2, "off_sub": 2, "off_mul": 1, "nf": np.int8},
    "uint16_t": {"off_add": 2, "off_sub": 2, "off_mul": 2, "nf": np.uint16},
    "int16_t": {"off_add": 2, "off_sub": 2, "off_mul": 2, "nf": np.int16},
    "uint32_t": {"off_add": 2, "off_sub": 2, "off_mul": 16, "nf": np.uint32},
    "int32_t": {"off_add": 2, "off_sub": 2, "off_mul": 16, "nf": np.int32},
    "uint64_t": {
        "off_add": 2, "off_sub": 3, "off_mul": 34, "off_div": 2,"nf":np.uint64},
    "int64_t": {
        "off_add": 2, "off_sub": 3, "off_mul": 34, "off_div": 2,"nf":np.int64},
    #"half": {"min": -1, "max": 1, "nf": np.float16, "nf_out": np.float32},
    "float": {"min": -1, "max": 1, "nf": np.float32, "nf_out": np.float32},
    "double": {"min": -1, "max": 1, "nf": np.float64, "nf_out": np.float64},
}

FP_C_MAP = {np.float16: "_Float16", np.float32: "float", np.float64: "double"}

def rnd_gen_1d_arr(min, max, len, dtype):
    return np.array(
        [random.uniform(min, max) for _ in range(len)], dtype=dtype)

def rnd_gen_2d_arr(min, max, rows, cols, dtype):
    return np.array(
        [[random.uniform(min, max)
          for _ in range(cols)]
          for _ in range(rows)],
        dtype=dtype)

def finish_gen(code, header, add_assert=True):
    if add_assert:
        code.append('#else')
        code.append('_Static_assert(0, "NF not defined");')
        code.append('#endif\n')

    with open(header, "w") as f:
        f.write("\n".join(code))
        print(f"Generated {header}")
