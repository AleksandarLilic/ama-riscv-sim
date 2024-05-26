import random
import numpy as np

# format numpy array as C array
def np2c(var, arr, dir="in", suffix=""):
    return f"NF_{dir.upper()} " + var + f"[ARR_LEN]" + \
            " = {" + f"{suffix}, ".join([f"{x}" for x in arr]) + suffix + "};"

NUM = {
    "uint8_t": {"nf_in": np.uint8, "nf_out": np.uint32},
    "int8_t": {"nf_in": np.int8, "nf_out": np.int32},
    "uint16_t": {"nf_in": np.uint16, "nf_out": np.uint32},
    "int16_t": {"nf_in": np.int16, "nf_out": np.int32},
    "uint32_t": {"off_add": 2, "off_sub": 2, "off_mul": 16,
                 "nf_in": np.uint32, "nf_out": np.uint32},
    "int32_t": {"off_add": 2, "off_sub": 2, "off_mul": 16,
                "nf_in": np.int32, "nf_out": np.int32},
    "uint64_t": {"off_add": 2, "off_sub": 3, "off_mul": 34, "off_div": 2,
                 "nf_in": np.uint64, "nf_out": np.uint64},
    "int64_t": {"off_add": 2, "off_sub": 3, "off_mul": 34, "off_div": 2,
                "nf_in": np.int64, "nf_out": np.int64},
    #"half": {"min": -1, "max": 1, "nf_in": np.float16, "nf_out": np.float32},
    "float": {"min": -1, "max": 1, "nf_in": np.float32, "nf_out": np.float32},
    "double": {"min": -1, "max": 1, "nf_in": np.float64, "nf_out": np.float64},
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
