"""Data generation for the A(M × K) @ B(K × N) -> ref(M × N) family.

Every dot-shaped test here is that product with dimensions:
    dotv M=1 N=1 | dotf M=MR N=1 | gemv M=m N=1 | gemm M,N,K

K is the inner/reduction dim.
A is row-major with row stride 'lda' in elements (>= K);
columns [K, lda) are padding no correct kernel reads
At N == 1, B is a contiguous K-vector.

'b_t' flips only B's storage layout, never the math: the reference is
a[:, :K] @ b[:K, :N] either way
It picks which axis of B is contiguous, and therefore which one 'ldb' strides
and where B's padding lands:
    b_t=0: K rows of n-contiguous data, ldb strides n, padding in [N, ldb)
    b_t=1: N rows of k-contiguous data, ldb strides k, padding in [K, ldb)

tests keep their own sweep list, #if/#define emission and output filename;
this module never emits C
"""

import argparse
import random
import zlib
from dataclasses import dataclass

import numpy as np
from codegen_common import (NUM, el_per_byte, el_per_word, n_bytes, nkey,
                            pack_narrow, rnd_gen_2d_arr, unpack_narrow)

INT32_MIN, INT32_MAX = -2**31, 2**31 - 1
MAX_BYTES = 64 * 1024 # default total size in B, mostly dcache size driven
INJECT_EVERY = 24 # 1 in N elements carries an injected high-magnitude value

# local helpers
def _max_mag(t):
    """get the abs max value: -MIN for int, MAX for uint"""
    return max(-NUM[nkey(t)]["min"], NUM[nkey(t)]["max"])

def _row_stride(k, t, ld_min=None):
    """>= k, rounded up so every row starts word-aligned for this type"""
    q = el_per_word(nkey(t))
    return ((max(k, ld_min or 0) + q - 1) // q) * q

def _unaligned_k(K, ta, tb):
    """largest k < K on a storage-byte boundary for both types: the 'ual'
    companion length that exercises the kernels' K tails"""
    q = max(el_per_byte(nkey(ta)), el_per_byte(nkey(tb)))
    return ((K - 1) // q) * q

def _shift(K, ta, tb, n_pairs):
    """shrink both ranges by whole bits until the worst case K*max|a|*max|b|
    fits, leaving room for the injected pairs' residuals\n
    depends only on K\n
    shrinks the wider side first,
    so a narrow type is never crushed against a wide one
    """
    budget = INT32_MAX - n_pairs * _max_mag(tb)
    amax, bmax = _max_mag(ta), _max_mag(tb)
    sa = sb = 0
    while (K * (amax >> sa) * (bmax >> sb)) > budget:
        if (amax >> sa) >= (bmax >> sb):
            sa += 1
        else:
            sb += 1
        if not (amax >> sa) or not (bmax >> sb):
            raise RuntimeError(f"cannot bound {ta}x{tb} at K={K}")
    return sa, sb

def _draw(rows, ld, used, t, shift):
    """draws random values in the range [min >> shift, max >> shift]
    for the first used columns, and [min, max] for the padding"""
    v = NUM[nkey(t)]
    out = np.zeros((rows, ld), dtype=np.int64)
    out[:, :used] = rnd_gen_2d_arr(
        (v["min"] >> shift), (v["max"] >> shift), rows, used, np.int64
    )
    if ld > used:
        # padding, out of range values are preferred since they are never read
        out[:, used:] = rnd_gen_2d_arr(
            v["min"], v["max"], rows, (ld - used), np.int64
        )
    return out

def _upper_band(t, shift):
    """magnitude in [shrunk_clamp, max] (top of type), random sign"""
    v = NUM[nkey(t)]
    existing_range = (-v["min"]) >> shift
    lower_lim = min(max(existing_range, 1), v["max"])
    upper_lim = v["max"]
    m = random.randint(lower_lim, upper_lim)
    return m if random.randint(0, 1) else -m

def _inject(a_log, b_log, K, ta, tb, sa, sb, n_pairs):
    """inject value pairs at word-aligned positions,
    so a shrunk range still toggles the wide bits:
      a[i][j] = v, a[i][j+1] = -(v-1), b[j] = b[j+1] = w  ->  residual w
    both elements sit in one SIMD word
    valid for every column of B, hence also at N > 1"""
    w = min(el_per_word(ta), el_per_word(tb))

    # whole pairs below the 'ual' cut, so the shorter ref is bounded too
    ulim = max(_unaligned_k(K, ta, tb) - 1, 0) # don't inject at ual tail (-1)
    starts = list(range(0, ulim, w)) # w step stops it at boundary, not ulim
    samples = min(n_pairs, len(starts))
    for j in sorted(random.sample(starts, samples)):
        bw = _upper_band(tb, sb)
        b_log[j, :] = b_log[j + 1, :] = bw # match the value on b, so a cancels
        for i in range(a_log.shape[0]):
            v = _upper_band(ta, sa)
            a_log[i, j] = v
            a_log[i, j + 1] = -(v - (1 if v > 0 else -1)) # bump v by 1 toward 0

def _pack_as_1d(log, t):
    v = NUM[nkey(t)]
    flat = log.reshape(-1)
    if "narrow_bits" in v:
        return pack_narrow(flat, v["narrow_bits"]).astype(v["nf"])
    return flat.astype(v["nf"])

def _unpack_to_2d(packed, t, rows, ld):
    v = NUM[nkey(t)]
    if "narrow_bits" in v:
        flat = unpack_narrow(
            packed, v["narrow_bits"], np.issubdtype(v["nf"], np.signedinteger)
        )
    else:
        flat = packed
    return np.asarray(flat, dtype=np.int64).reshape(rows, ld)

# datagen
@dataclass
class data:
    a: np.ndarray # packed storage, flat
    b: np.ndarray
    a_log: np.ndarray # (M, lda) logical, padding included
    b_log: np.ndarray # logical in (k, n) order, padding included
    b_sl: np.ndarray # b_log in storage layout: what 'b' packs and C declares
    lda: int
    ldb: int
    ref: np.ndarray # (M, N)
    shift: tuple # bits the ranges were shrunk by
    M: int
    N: int
    K: int
    k_ual: int # largest k < K on a byte boundary, for the 'ual' companion ref
    ta: str
    tb: str

    @property
    def b_t(self):
        """the 'b_t' gen() was called with:
        b_sl is a transposed view of b_log"""
        return self.b_log is not self.b_sl

    def ref_at(self, k):
        """reference for a shorter reduction, e.g. the 'ual' length"""
        return self.a_log[:, :k] @ self.b_log[:k, :self.N]

def gen(
    M, N, K,
    type_a, type_b,
    lda=None, ldb=None, b_t=False,
    overflow_check=True, max_bytes=MAX_BYTES, seed_in=1
):
    ta, tb = nkey(type_a), nkey(type_b)
    q = max(el_per_byte(ta), el_per_byte(tb))
    if K % q:
        raise ValueError(f"K={K} must be a multiple of {q} for {ta}/{tb}")

    lda = _row_stride(K, ta, lda)
    # 'ldb' strides whichever axis is the slow one, so its meaning follows b_t
    if b_t:
        ldb = _row_stride(K, tb, ldb)
        b_rows, b_used = N, K
    else:
        ldb = 1 if (N == 1) else _row_stride(N, tb, ldb)
        b_rows, b_used = K, N
    data_req = {
        "a": n_bytes(M * lda, ta),
        "b": n_bytes(b_rows * ldb, tb),
        "c": n_bytes(M * N, "int32_t"),
        "ref": n_bytes(M * N, "int32_t"),
    }
    data_req_sum = sum(data_req.values())
    if data_req_sum > max_bytes:
        raise ValueError(
            f"Total data required is {data_req_sum} B, over {max_bytes} B - "
            f"a={data_req['a']} B, b={data_req['b']} B, "
            f"c={data_req['c']} B, ref={data_req['ref']} B"
        )

    # reproducible seeds; adding a type pair later doesn't change existing gen
    random.seed(zlib.crc32(f"{ta}:{tb}:{M}:{N}:{K}:{seed_in}".encode()))

    # at least one pair whenever there is room
    n_pairs = max(1, K // (2 * INJECT_EVERY))
    sa, sb = _shift(K, ta, tb, n_pairs) if overflow_check else (0, 0)

    a_log = _draw(M, lda, K, ta, sa)
    # everything downstream reads b in (k, n) order;
    # '.T' is a view, so injecting through it lands in the storage layout also
    b_sl = _draw(b_rows, ldb, b_used, tb, sb)
    b_log = b_sl.T if b_t else b_sl
    if sa or sb: # put the wide bits back, only if originally shifted down
        _inject(a_log, b_log, K, ta, tb, sa, sb, n_pairs)

    ref = a_log[:, :K] @ b_log[:K, :N] # the rest of either axis is padding
    ref_ib = (INT32_MIN <= ref.min()) and (ref.max() <= INT32_MAX) # in bounds
    if overflow_check and not ref_ib:
        raise RuntimeError(
            f"{ta} x {tb} K={K}: ref outside int32 at shift {(sa, sb)}"
        )

    return data(
        _pack_as_1d(a_log, ta), _pack_as_1d(b_sl, tb),
        a_log, b_log, b_sl, lda, ldb,
        ref, (sa, sb), M, N, K, _unaligned_k(K, ta, tb), ta, tb
    )

def self_check(d):
    # re-derive the ref from the storage actually emitted:
    # cheap check catching only packing, stride, and padding mistakes
    a = _unpack_to_2d(d.a, d.ta, d.M, d.lda)
    b = _unpack_to_2d(d.b, d.tb, d.b_sl.shape[0], d.ldb)
    assert np.array_equal(a, d.a_log), f"{d.ta}: 'a' does not round-trip"
    assert np.array_equal(b, d.b_sl), f"{d.tb}: 'b' does not round-trip"
    # back to standard view, so the ref goes through the emitted layout
    b_log = b.T if d.b_t else b
    assert np.array_equal(a[:, :d.K] @ b_log[:d.K, :d.N], d.ref), "ref mismatch"

def add_dim_args(parser):
    """common data dimensions argumets"""
    def p_int(s):
        v = int(s)
        if v < 1:
            raise argparse.ArgumentTypeError(f"{v}: must be >= 1")
        return v

    parser.add_argument("-M", type=p_int, default=1, help="rows of A / outputs")
    parser.add_argument("-N", type=p_int, default=1, help="cols of B")
    parser.add_argument("-K", type=p_int, default=1, help="reduction length")
    parser.add_argument("--lda", type=p_int, default=None, help="min row stride of A in elements, rounded up per type")
    parser.add_argument("--ldb", type=p_int, default=None, help="min row stride of B in elements, rounded up per type; strides n by default (ignored at N = 1), k under --b_t")
    parser.add_argument("--max_bytes", type=p_int, default=MAX_BYTES)
    parser.add_argument("--seed", type=int, default=1, help="unique input seed")
    parser.add_argument("--no_overflow_check", action="store_true", help="let refs wrap int32 instead of bounding the operand ranges")
