import random
import numpy as np

# format numpy array as C array
def np2c(var, arr, suffix=""):
    return f"NF " + var + f"[ARR_LEN]" + \
            " = {" + f"{suffix}, ".join([f"{x}" for x in arr]) + suffix + "};"

NUM = {
    "uint8_t": {"off_add": 2, "off_sub": 2, "off_mul": 4, "nf": np.uint8},
    "int8_t": {"off_add": 2, "off_sub": 2, "off_mul": 4, "nf": np.int8},
    "uint16_t": {"off_add": 2, "off_sub": 2, "off_mul": 8, "nf": np.uint16},
    "int16_t": {"off_add": 2, "off_sub": 2, "off_mul": 8, "nf": np.int16},
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

def rnd_gen(min, max, len, dtype):
    return np.array([random.uniform(min, max) for _ in range(len)],
                    dtype=dtype)

def finish_gen(code, OUT):
    code.append('#else')
    code.append('_Static_assert(0, "NF not defined");')
    code.append('#endif\n')

    with open(OUT, "w") as f:
        f.write("\n".join(code))
        print(f"Generated {OUT}")
