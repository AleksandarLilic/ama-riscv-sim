import argparse
import random

import numpy as np


def align_attr(align):
    """align:\n
    None -> no attribute\n
    int  -> byte count, wrapped as: 4 -> __attribute__((aligned(4)))\n
    str  -> a C-side macro, pasted as-is: "A_ALIGN" -> A_ALIGN"""
    if align is None:
        return ""
    if isinstance(align, str):
        return f" {align.strip()}"
    return f" __attribute__((aligned({align})))"

def np2c_1d_arr(
    var, arr, nf="NF", dim="ARR_LEN",
    align=None, suffix="", str_type=False, row_len=None
):
    if str_type:
        arr_out = [f"\"{x}\"" for x in arr]
    else:
        arr_out = [f"{x}" for x in arr]

    decl = f"{nf} " + var + f"[{dim}]{align_attr(align)}" + " = {"
    if not row_len:
        return decl + f"{suffix}, ".join(arr_out) + suffix + "};"

    # row_len: C header file entries per line for readibility, None for 1 line
    rows = [arr_out[i:i + row_len] for i in range(0, len(arr_out), row_len)]
    return decl + "\n    " + ",\n    ".join(
        f"{suffix}, ".join(r) + suffix for r in rows) + "\n};"

def np2c_2d_arr(var, arr, nf="NF", dim=["A", "B"], align=None, suffix=""):
    arr_out = []
    for row in arr:
        arr_out.append("\n    {" + ", ".join([f"{x}" for x in row]) + "}")

    return f"{nf} " + var + f"[{dim[0]}][{dim[1]}]{align_attr(align)}" + \
        " = {" + f"{suffix}, ".join(arr_out) + suffix + "\n};"

NUM = {
    "uint8_t":  { "offset": {"add": 2, "sub": 2, "mul": 3, "wmul": 1},             "nf": np.uint8},
    "int8_t":   { "offset": {"add": 2, "sub": 2, "mul": 3, "wmul": 1},             "nf": np.int8},
    "uint16_t": { "offset": {"add": 2, "sub": 2, "mul": 7, "wmul": 2},             "nf": np.uint16},
    "int16_t":  { "offset": {"add": 2, "sub": 2, "mul": 7, "wmul": 2},             "nf": np.int16},
    "uint32_t": { "offset": {"add": 2, "sub": 2, "mul": 24, "wmul": 16},           "nf": np.uint32},
    "int32_t":  { "offset": {"add": 2, "sub": 2, "mul": 24, "wmul": 16},           "nf": np.int32},
    "uint64_t": { "offset": {"add": 2, "sub": 3, "mul": 40, "wmul": 34, "div": 2}, "nf": np.uint64},
    "int64_t":  { "offset": {"add": 2, "sub": 3, "mul": 40, "wmul": 34, "div": 2}, "nf": np.int64},
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

def nkey(t):
    """convenience, pass int8 or int8_t, both resolve to NUM key"""
    return t if t in NUM else f"{t}_t"

# element geometry, derived from NUM
def bits_per_el(t):
    """logical bits per element: int16 -> 16, int8 -> 8, int4 -> 4, int2 -> 2"""
    value = NUM[t]
    if "narrow_bits" in value:
        return value["narrow_bits"]
    return 8 * np.dtype(value["nf"]).itemsize

def el_per_byte(t):
    """elements sharing one storage byte; 1 for anything byte-wide or wider"""
    return max(1, 8 // bits_per_el(t))

def el_per_word(t):
    """elements in one 32-bit word\n
    doubles as the row-stride slice: a 2D panel whose stride is a multiple of
    this has every row starting on a 4-byte boundary,
    which -mstrict-align requires for the SIMD/LOAD_OPT word loads"""
    return (32 // bits_per_el(t))

def n_bytes(n_el, t):
    """storage bytes for n_el logical elements"""
    return (n_el * bits_per_el(t) // 8)

def storage_dim(el_expr, t):
    """C dim expression for storage, given one for the logical element count:\n
    'M*LDA' -> 'M*LDA' (int8/int16), '(M*LDA)>>1' (int4), '(M*LDA)>>2' (int2)"""
    shift = {16: 0, 8: 0, 4: 1, 2: 2}[bits_per_el(t)]
    return el_expr if not shift else f"({el_expr})>>{shift}"

def row_stride(k, t, ld_min=None):
    """>= k, rounded up so every row starts word-aligned for this type"""
    q = el_per_word(nkey(t))
    return ((max(k, ld_min or 0) + q - 1) // q) * q

def draw_rnd(rows, ld, used, t, shift=0):
    """draws random values in the range [min >> shift, max >> shift]
    for the first used columns, and [min, max] for the padding"""
    v = NUM[nkey(t)]
    out = np.zeros((rows, ld), dtype=np.int64)
    out[:, :used] = rnd_gen_2d_arr(
        (v["min"] >> shift), (v["max"] >> shift), rows, used, np.int64
    )
    if ld > used:
        # padding, out of range values are fine since they are never read
        out[:, used:] = rnd_gen_2d_arr(
            v["min"], v["max"], rows, (ld - used), np.int64
        )
    return out

def pack_lanes(lanes, t):
    """logical lanes -> storage entries, the representation change only\n
    narrow types get bit-packed, others stay the same\n
    output is flat because storage is a sequence; reshaping is the caller's"""
    v = NUM[nkey(t)]
    flat = np.asarray(lanes).reshape(-1)
    if "narrow_bits" in v:
        return pack_narrow(flat, v["narrow_bits"]).astype(v["nf"])
    return flat.astype(v["nf"])

def unpack_lanes(storage, t):
    """storage entries -> logical lanes, flat; the inverse of pack_lanes"""
    v = NUM[nkey(t)]
    if "narrow_bits" in v:
        return unpack_narrow(
            storage, v["narrow_bits"], np.issubdtype(v["nf"], np.signedinteger)
        )
    return np.asarray(storage, dtype=np.int64)

# shared codegen config and CLI
MAX_BYTES = 64 * 1024 # default total size in B, mostly dcache size driven
ALIGN = 4 # word loads on the SIMD/LOAD_OPT paths, -mstrict-align

def _p_int(s):
    v = int(s)
    if v < 1:
        raise argparse.ArgumentTypeError(f"{v}: must be >= 1")
    return v

def add_common_args(parser):
    """what every datagen needs regardless of shape"""
    parser.add_argument("--max_bytes", type=_p_int, default=MAX_BYTES)
    parser.add_argument("--seed", type=int, default=1, help="unique input seed")

POISON = {32: 0xDEADBEEF, 16: 0xBEEF, 8: 0xBE}

def poison_arr(n_el, nf):
    bits = 8 * np.dtype(nf).itemsize
    v = POISON[bits]
    # wrap into the signed range so values are meaningful downstream (numpy & C)
    if np.issubdtype(nf, np.signedinteger) and v >= (1 << (bits - 1)):
        v -= (1 << bits)
    return np.full(n_el, v, dtype=nf)

def iter_num(*kinds, narrow=None):
    """return iterator over NUM for the given 'kinds', w/ or w/o narrow types"""
    return (
        (key, value)
        for key, value in NUM.items()
        if ((not kinds) or (value["kind"] in kinds)) and
            ((narrow is None) or ("narrow_bits" in value) == narrow)
    )

# randint is inclusive, so both endpoints stay reachable;
# uniform+cast truncates toward zero and can never produce the most negative val
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

def pack_narrow(arr, bits):
    """packed (narrow) types: (u)int4/(u)int2 have no native numpy/C type,
    so the logical lanes are packed into (u)int8 storage;\n
    lane 0 in the low bits (LSB-first), matching SIMD ISA definition"""
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

def rnd_gen_1d_arr_narrow(value, len, vmin=None, vmax=None):
    """value is a NUM entry with 'narrow_bits' (e.g. NUM["int4_t"])\n
    returns (logical, packed): logical has `len` lanes (dtype value['nf']),
    packed has len // (8 // narrow_bits) bytes (dtype value['nf']).
    vmin/vmax override the type's own range, for callers that must shrink it"""
    arr = rnd_gen_1d_arr(
        value["min"] if vmin is None else vmin,
        value["max"] if vmax is None else vmax,
        len, value["nf"]
    )
    packed = pack_narrow(arr, value["narrow_bits"]).astype(value["nf"])
    return arr, packed

def np2c_1d_arr_narrow(
    var, arr, packed, nf="NF", dim="ARR_LEN", dim_packed=None,
    align=None, suffix="", row_len=None, add_comment=True
):
    """Emit '// actual var[dim] = {...}' (logical lanes) then the packed
    C array. dim_packed defaults to 'dim>>1' (4-bit) / 'dim>>2' (2-bit).
    row_len is in packed C entries, not logical lanes."""
    per_byte = len(arr) // len(packed)
    if dim_packed is None:
        dim_packed = f"{dim}>>{per_byte // 2}"

    comment = ""
    if add_comment:
        comment = \
            f"// actual {var}[{dim}] = " + \
            "{" + ", ".join(str(int(x)) for x in arr) + "};" + "\n"
    return comment + \
        np2c_1d_arr(
            var, packed, nf=nf, dim=dim_packed, align=align, suffix=suffix,
            row_len=row_len
        )

def np2c_2d_arr_narrow(
    var, arr, packed, nf="NF", dim=["A", "B"], dim_packed=None,
    align=None, suffix="", add_comment=True
):
    """2D counterpart of np2c_1d_arr_narrow: '// actual var[d0][d1] = {...}'
    (logical lanes, one row per line) then the packed C array.
    arr is (rows, cols) logical, packed is (rows, cols // lanes-per-byte)."""
    per_byte = arr.shape[1] // packed.shape[1]
    if dim_packed is None:
        dim_packed = [dim[0], f"{dim[1]}>>{per_byte // 2}"]

    comment = ""
    if add_comment:
        comment = \
            f"// actual {var}[{dim[0]}][{dim[1]}] = {{" + \
            "".join("\n//     {" + \
                ", ".join(str(int(x)) for x in row) + "}," for row in arr) + \
            "\n// };" + "\n"

    return comment + \
        np2c_2d_arr(
            var, packed, nf=nf, dim=dim_packed, align=align, suffix=suffix
        )

def emit_panel(
    name, packed, t, dim_el, dim_2d, ld,
    align=None, flatten=True, qual="", add_comment=True
):
    """typed panel: flat 'pointer + stride' by default, 2D when not flatten\n
    sub-byte types emitted packed, logical lanes kept alongside as comment"""
    value = NUM[t]
    nf = qual + value["ctype"]
    is_narrow = "narrow_bits" in value
    rows = (len(packed) * el_per_byte(t)) // ld
    logical = unpack_lanes(packed, t).reshape(rows, ld)

    dim_2d_packed = [dim_2d[0], storage_dim(dim_2d[1], t)]
    if not flatten:
        if not is_narrow:
            return np2c_2d_arr(
                name, logical, nf=nf, dim=dim_2d_packed, align=align
            )
        # a packed row is always a whole number of bytes, so the flat packing
        # reshapes into rows as is - no lane straddles a row boundary
        rows, per_row = logical.shape[0], (ld // el_per_byte(t))
        return np2c_2d_arr_narrow(
            name, logical, packed.reshape(rows, per_row), nf=nf,
            dim=dim_2d, dim_packed=dim_2d_packed, align=align,
            add_comment=add_comment
        )

    # storage entries per emitted line, purely for readability, 2d shape only
    row_len = ld // el_per_byte(t)
    row_len = row_len if (row_len > 1) else None

    if not is_narrow:
        return np2c_1d_arr(
            name, packed, nf=nf, dim=storage_dim(dim_el, t),
            align=align, row_len=row_len
        )
    return np2c_1d_arr_narrow(
        name, logical.reshape(-1), packed, nf=nf, dim=dim_el,
        dim_packed=storage_dim(dim_el, t), align=align, row_len=row_len,
        add_comment=add_comment
    )

def finish_gen(code, header, add_assert=True):
    if add_assert:
        code.append('#else')
        code.append('_Static_assert(0, "NF not defined");')
        code.append('#endif\n')

    with open(header, "w") as f:
        f.write("\n".join(code))
        print(f"Generated {header}")
