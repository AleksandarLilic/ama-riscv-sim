import random
import numpy as np

def np2c_1d_arr(
    var, arr, nf="NF", dim="ARR_LEN", align="", suffix="", str_type=False):
    if str_type:
        arr_out = [f"\"{x}\"" for x in arr]
    else:
        arr_out = [f"{x}" for x in arr]

    return f"{nf} " + var + f"[{dim}]{align}" + " = {" + \
        f"{suffix}, ".join(arr_out) + suffix + "};"

def np2c_2d_arr(var, arr, nf="NF", dim=["A", "B"], suffix=""):
    arr_out = []
    for row in arr:
        arr_out.append("\n    {" + ", ".join([f"{x}" for x in row]) + "}")

    return f"{nf} " + var + f"[{dim[0]}][{dim[1]}]" + " = {" + \
        f"{suffix}, ".join(arr_out) + suffix + "\n};"

NUM = {
    "uint8_t":  { "offset": {"add": 2, "sub": 2, "mul": 1},            "nf": np.uint8},
    "int8_t":   { "offset": {"add": 2, "sub": 2, "mul": 1},            "nf": np.int8},
    "uint16_t": { "offset": {"add": 2, "sub": 2, "mul": 2},            "nf": np.uint16},
    "int16_t":  { "offset": {"add": 2, "sub": 2, "mul": 2},            "nf": np.int16},
    "uint32_t": { "offset": {"add": 2, "sub": 2, "mul": 16},           "nf": np.uint32},
    "int32_t":  { "offset": {"add": 2, "sub": 2, "mul": 16},           "nf": np.int32},
    "uint64_t": { "offset": {"add": 2, "sub": 3, "mul": 34, "div": 2}, "nf": np.uint64},
    "int64_t":  { "offset": {"add": 2, "sub": 3, "mul": 34, "div": 2}, "nf": np.int64},
    #"half":   {"min": -1, "max": 1, "nf": np.float16},
    "float":  {"min": -1, "max": 1, "nf": np.float32},
    "double": {"min": -1, "max": 1, "nf": np.float64},
}

FP_C_MAP = {np.float16: "_Float16", np.float32: "float", np.float64: "double"}

# fill in the fields
for key, value in NUM.items():
    nf = value["nf"]
    if "min" in value: # min/max already set for floats
        value["kind"] = "fp"
        value["macro"] = nf.__name__.upper() # "float32" -> "FLOAT32"
        value["ctype"] = FP_C_MAP[nf]
    else:
        value["min"] = np.iinfo(nf).min
        value["max"] = np.iinfo(nf).max
        value["kind"] = "int" if np.issubdtype(nf, np.signedinteger) else "uint"
        value["macro"] = nf.__name__.upper() # "int8" -> "INT8"
        value["ctype"] = key

def iter_num(*kinds, narrow=None):
    return (
        (key, value)
        for key, value in NUM.items()
        if (not kinds or value["kind"] in kinds)
        and (
            narrow is None
            or ("narrow_bits" in value) == narrow
        )
    )

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
