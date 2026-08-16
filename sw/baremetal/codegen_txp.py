"""Data generation for the transpose family: c(N x M) = a(M x N)^T

both panels are row-major with a row stride in elements:
'a' is M rows of N used elements strided by 'lda',
'c' is N rows of M strided by 'ldc'
columns past the used ones are padding that no correct kernel touches

the reference is emitted at the destination's exact shape and stride

tests keep their own sweep list, #if/#define and output filename;
this module never emits C
"""

import random
import zlib
from dataclasses import dataclass

import numpy as np
from codegen_common import (MAX_BYTES, NUM, _p_int, add_common_args, draw_rnd,
                            el_per_byte, el_per_word, n_bytes, nkey,
                            pack_lanes, poison_arr, row_stride, unpack_lanes)


@dataclass
class data:
    a: np.ndarray # packed storage, flat
    c: np.ndarray # poisoned destination, packed storage, flat
    ref: np.ndarray # a^T at c's shape and stride, poison in the padding
    a_log: np.ndarray # (M, lda) logical, padding included
    lda: int
    ldc: int
    M: int
    N: int
    t: str

def gen(M, N, type_a, lda=None, ldc=None, max_bytes=MAX_BYTES, seed_in=1):
    t = nkey(type_a)
    blk = el_per_word(t) # the block is forced square, at elements per word
    if (M % blk) or (N % blk):
        raise ValueError(f"M={M} and N={N} must be multiples of {blk} for {t}")

    lda = row_stride(N, t, lda)
    ldc = row_stride(M, t, ldc)
    data_req = {
        "a": n_bytes(M * lda, t),
        "c": n_bytes(N * ldc, t),
        "ref": n_bytes(N * ldc, t),
    }
    data_req_sum = sum(data_req.values())
    if data_req_sum > max_bytes:
        raise ValueError(
            f"Total data required is {data_req_sum} B, over {max_bytes} B - "
            + ", ".join(f"{k}={v} B" for k, v in data_req.items())
        )

    # reproducible seeds; adding a type later doesn't change existing gen
    random.seed(zlib.crc32(f"{t}:{M}:{N}:{seed_in}".encode()))

    a_log = draw_rnd(M, lda, N, t)

    # 'c' and 'ref' start fully poisoned
    per_row = ldc // el_per_byte(t) # C entries per row, not logical elements
    used = M // el_per_byte(t)
    c = poison_arr(N * per_row, NUM[t]["nf"])
    ref = poison_arr(N * per_row, NUM[t]["nf"])
    ref_t = a_log[:M, :N].T # (N, M), the used region
    for r in range(N):
        ref[r*per_row:(r*per_row + used)] = pack_lanes(ref_t[r], t)

    return data(pack_lanes(a_log, t), c, ref, a_log, lda, ldc, M, N, t)

def self_check(d):
    # re-derive the transpose from the storage actually emitted:
    # cheap check catching only packing, stride, and padding mistakes
    a = unpack_lanes(d.a, d.t).reshape(d.M, d.lda)
    ref = unpack_lanes(d.ref, d.t).reshape(d.N, d.ldc)
    assert np.array_equal(a, d.a_log), f"{d.t}: 'a' does not round-trip"
    assert np.array_equal(ref[:, :d.M], a[:d.M, :d.N].T), "ref is not a^T"

    # the padding of 'ref' has to match 'c' exactly, so main.c can compare the
    # whole buffer and catch a kernel that writes outside its block
    per_row = d.ldc // el_per_byte(d.t)
    used = d.M // el_per_byte(d.t)
    for r in range(d.N):
        pad = slice(r*per_row + used, (r + 1)*per_row)
        assert np.array_equal(d.ref[pad], d.c[pad]), "ref padding is not poison"

def add_dim_args(parser):
    """dimensions of the transpose flavor"""
    p_int = _p_int
    parser.add_argument("-M", type=p_int, default=1, help="rows of the source")
    parser.add_argument("-N", type=p_int, default=1, help="cols of the source")
    parser.add_argument("--lda", type=p_int, default=None, help="min row stride of the source in elements, rounded up per type")
    parser.add_argument("--ldc", type=p_int, default=None, help="min row stride of the destination in elements, rounded up per type")
    add_common_args(parser)
