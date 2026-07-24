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
    # packed types
    "uint4_t": {"narrow_bits": 4, "nf": np.uint8},
    "int4_t":  {"narrow_bits": 4, "nf": np.int8},
    "uint2_t": {"narrow_bits": 2, "nf": np.uint8},
    "int2_t":  {"narrow_bits": 2, "nf": np.int8},
}

FP_C_MAP = {np.float16: "_Float16", np.float32: "float", np.float64: "double"}

# fill in the fields
for key, value in NUM.items():
    nf = value["nf"]
    if "narrow_bits" in value:
        bits = value["narrow_bits"]
        signed = np.issubdtype(nf, np.signedinteger)
        value["min"] = -(1 << (bits - 1)) if signed else 0
        value["max"] = (1 << (bits - 1)) - 1 if signed else (1 << bits) - 1
        value["kind"] = "int" if signed else "uint"
        value["macro"] = key.upper().removesuffix("_T") # "int4_t" -> "INT4"
        value["ctype"] = value["kind"] + "8_t" # ctype always 8-bit
    elif "min" in value: # min/max already set for floats
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

# randint is inclusive, so both endpoints stay reachable; uniform+cast truncates
# toward zero and can never produce the most negative value of the type
def rnd_fn(dtype):
    return random.randint if np.issubdtype(dtype, np.integer) else random.uniform

def rnd_gen_1d_arr(min, max, len, dtype):
    rnd = rnd_fn(dtype)
    return np.array(
        [rnd(min, max) for _ in range(len)], dtype=dtype)

def rnd_gen_2d_arr(min, max, rows, cols, dtype):
    rnd = rnd_fn(dtype)
    return np.array(
        [[rnd(min, max)
          for _ in range(cols)]
          for _ in range(rows)],
        dtype=dtype)

# packed (narrow) types: (u)int4/(u)int2 have no native numpy/C type, so the
# logical lanes are packed into (u)int8 storage, lane 0 in the low bits
# (LSB-first), matching PK_SB_I4/PK_SB_I2 and v_load_int4x8/v_load_int2x16 in
# common/c_test_common.h and common/common_math.h.
def pack_narrow(arr, bits):
    per_byte = 8 // bits
    assert len(arr) % per_byte == 0, \
        f"length {len(arr)} not a multiple of {per_byte} for {bits}-bit lanes"
    mask = (1 << bits) - 1
    packed = np.zeros(len(arr) // per_byte, dtype=np.uint8)
    for i, x in enumerate(arr):
        packed[i // per_byte] |= (int(x) & mask) << ((i % per_byte) * bits)
    return packed

def unpack_narrow(packed, bits, signed):
    per_byte = 8 // bits
    mask = (1 << bits) - 1
    sign_bit = 1 << (bits - 1)
    arr = []
    for byte in packed:
        for lane in range(per_byte):
            x = (int(byte) >> (lane * bits)) & mask
            if signed and (x & sign_bit):
                x -= (1 << bits)
            arr.append(x)
    return np.array(arr)

def rnd_gen_1d_arr_narrow(value, len):
    """value is a NUM entry with 'narrow_bits' (e.g. NUM["int4_t"]).
    Returns (logical, packed): logical has `len` lanes (dtype value['nf']),
    packed has len // (8 // narrow_bits) bytes (dtype value['nf'])."""
    arr = rnd_gen_1d_arr(value["min"], value["max"], len, value["nf"])
    packed = pack_narrow(arr, value["narrow_bits"]).astype(value["nf"])
    return arr, packed

def np2c_1d_arr_narrow(
    var, arr, packed, nf="NF", dim="ARR_LEN", dim_packed=None, align="",
    suffix=""):
    """Emit '// actual var[dim] = {...}' (logical lanes) then the packed
    C array. dim_packed defaults to 'dim>>1' (4-bit) / 'dim>>2' (2-bit)."""
    per_byte = len(arr) // len(packed)
    if dim_packed is None:
        dim_packed = f"{dim}>>{per_byte // 2}"
    comment = f"// actual {var}[{dim}] = " + \
        "{" + ", ".join(str(int(x)) for x in arr) + "};"
    return comment + "\n" + \
        np2c_1d_arr(var, packed, nf=nf, dim=dim_packed, align=align,
                    suffix=suffix)

def finish_gen(code, header, add_assert=True):
    if add_assert:
        code.append('#else')
        code.append('_Static_assert(0, "NF not defined");')
        code.append('#endif\n')

    with open(header, "w") as f:
        f.write("\n".join(code))
        print(f"Generated {header}")
