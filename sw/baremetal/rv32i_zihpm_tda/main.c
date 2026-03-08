#include <stdint.h>
#include "common.h"
#include "common_math.h"

#define CHECK(val, expected, csr_num) \
    if (val != expected){ \
        write_mismatch(val, expected, csr_num); \
        fail(); \
    }

#define CHECK_CSR(csr_addr, val) \
    write_csr(csr_addr, 0); \
    read_csr(csr_addr, rval); \
    CHECK(rval, 0, csr_addr);

int32_t sum_up(uint32_t* array, uint32_t length) {
    int32_t sum = 0;
    for (uint32_t i = 0; i < length; i++) sum += array[i];
    return sum;
}

void main() {
    #define LEN 512
    uint32_t arr[LEN];
    uint8_t arr_dot[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    #define STEP 128

    // check writing a random value, then check 0 (also serving as init)
    uint32_t rval;
    CHECK_CSR(CSR_MSCRATCH, 872);
    CHECK_CSR(CSR_MSCRATCH, 0);
    CHECK_CSR(CSR_MHPMCOUNTER3, 123);
    CHECK_CSR(CSR_MHPMCOUNTER3, 0);
    CHECK_CSR(CSR_MHPMCOUNTER4, 423);
    CHECK_CSR(CSR_MHPMCOUNTER4, 0);
    CHECK_CSR(CSR_MHPMCOUNTER5, 54);
    CHECK_CSR(CSR_MHPMCOUNTER5, 0);
    CHECK_CSR(CSR_MHPMCOUNTER6, 167);
    CHECK_CSR(CSR_MHPMCOUNTER6, 0);
    CHECK_CSR(CSR_MHPMCOUNTER7, 390);
    CHECK_CSR(CSR_MHPMCOUNTER7, 0);
    CHECK_CSR(CSR_MHPMCOUNTER8, 23);
    CHECK_CSR(CSR_MHPMCOUNTER8, 0);

    // set up events
    uint32_t ev;
    ev = mhpmevent_bad_spec;
    write_csr(CSR_MHPMEVENT3, ev);
    read_csr(CSR_MHPMEVENT3, rval);
    CHECK(rval, ev, CSR_MHPMEVENT3);

    ev = mhpmevent_fe;
    write_csr(CSR_MHPMEVENT4, ev);
    read_csr(CSR_MHPMEVENT4, rval);
    CHECK(rval, ev, CSR_MHPMEVENT4);

    ev = mhpmevent_be;
    write_csr(CSR_MHPMEVENT5, ev);
    read_csr(CSR_MHPMEVENT5, rval);
    CHECK(rval, ev, CSR_MHPMEVENT5);

    ev = mhpmevent_fe_ic;
    write_csr(CSR_MHPMEVENT6, ev);
    read_csr(CSR_MHPMEVENT6, rval);
    CHECK(rval, ev, CSR_MHPMEVENT6);

    ev = mhpmevent_be_dc;
    write_csr(CSR_MHPMEVENT7, ev);
    read_csr(CSR_MHPMEVENT7, rval);
    CHECK(rval, ev, CSR_MHPMEVENT7);

    ev = mhpmevent_ret_simd;
    write_csr(CSR_MHPMEVENT8, ev);
    read_csr(CSR_MHPMEVENT8, rval);
    CHECK(rval, ev, CSR_MHPMEVENT8);

    GLOBAL_SYMBOL("test_start");
    // generate some branch misses for mhpmcounter3
    uint32_t random = 0x74321239;
    bool check;
    int32_t cnt = 0;
    for (int i = 0; i < 32; i++) {
        check = (random >> i) & 0x1;
        if (check) cnt += 17;
        else cnt -= 7;
    }

    // iterate over array to generate some memory accesses
    int32_t small_sum;
    for (uint32_t i = 0; i < LEN; i+=STEP) {
        arr[i] = i * arr[i%(STEP<<1)];
        arr[i] -= 7; // should cause hazard on mul (if mul is 2 clks)
        small_sum += arr[i] >> 3;
    }

    int32_t sum = sum_up(arr, LEN);
    sum += small_sum;

    // generate some simd instructions
    int32_t dot_result = _simd_dot_product_int8(arr_dot, arr_dot, 8);
    sum += dot_result;

    // check counters, shouldn't be zero at this point
    read_csr(CSR_MHPMCOUNTER3, rval);
    if (rval == 0) {
        write_mismatch(rval, cnt, CSR_MHPMCOUNTER3);
        fail();
    }

    read_csr(CSR_MHPMCOUNTER4, rval);
    if (rval == 0) {
        write_mismatch(rval, cnt, CSR_MHPMCOUNTER4);
        fail();
    }

    read_csr(CSR_MHPMCOUNTER5, rval);
    if (rval == 0) {
        write_mismatch(rval, sum, CSR_MHPMCOUNTER5);
        fail();
    }

    read_csr(CSR_MHPMCOUNTER6, rval);
    if (rval == 0) {
        write_mismatch(rval, sum, CSR_MHPMCOUNTER6);
        fail();
    }

    read_csr(CSR_MHPMCOUNTER7, rval);
    if (rval == 0) {
        write_mismatch(rval, sum, CSR_MHPMCOUNTER7);
        fail();
    }

    read_csr(CSR_MHPMCOUNTER8, rval);
    if (rval == 0) {
        write_mismatch(rval, sum, CSR_MHPMCOUNTER8);
        fail();
    }

    pass();
}
