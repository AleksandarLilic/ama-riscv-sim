#include <stdint.h>
#include "common.h"
#include "common_math.h"

#define CHECK(val, expected, csr_num) \
    if (val != expected){ \
        write_mismatch(val, expected, csr_num); \
        fail(); \
    }

#define LEN 512
uint32_t arr[LEN] = {0};
uint8_t arr_dot[8] = {1, 2, 3, 4, 5, 6, 7, 8};
#define STEP 128

int32_t sum_up(uint32_t* array, uint32_t length) {
    int32_t sum = 0;
    for (uint32_t i = 0; i < length; i++) sum += array[i];
    return sum;
}

void main() {
    uint32_t expected, rval;

    // mscratch
    rval = read_csr(CSR_MSCRATCH);
    expected = 0x133;
    write_csr(CSR_MSCRATCH, expected);
    rval = read_csr(CSR_MSCRATCH);
    CHECK(rval, expected, CSR_MSCRATCH);

    // mhpmcounters, 0 at this point before events are set up
    expected = 0;
    rval = read_csr(CSR_MHPMCOUNTER3);
    CHECK(rval, expected, CSR_MHPMCOUNTER3);

    expected = 0;
    rval = read_csr(CSR_MHPMCOUNTER4);
    CHECK(rval, expected, CSR_MHPMCOUNTER4);

    expected = 0;
    rval = read_csr(CSR_MHPMCOUNTER5);
    CHECK(rval, expected, CSR_MHPMCOUNTER5);

    expected = 0;
    rval = read_csr(CSR_MHPMCOUNTER6);
    CHECK(rval, expected, CSR_MHPMCOUNTER6);

    expected = 0;
    rval = read_csr(CSR_MHPMCOUNTER7);
    CHECK(rval, expected, CSR_MHPMCOUNTER7);

    expected = 0;
    rval = read_csr(CSR_MHPMCOUNTER8);
    CHECK(rval, expected, CSR_MHPMCOUNTER8);

    // set up events
    rval = read_csr(CSR_MHPMEVENT3);
    expected = mhpmevent_bad_spec;
    write_csr(CSR_MHPMEVENT3, expected);
    rval = read_csr(CSR_MHPMEVENT3);
    CHECK(rval, expected, CSR_MHPMEVENT3);

    rval = read_csr(CSR_MHPMEVENT4);
    expected = mhpmevent_fe;
    write_csr(CSR_MHPMEVENT4, expected);
    rval = read_csr(CSR_MHPMEVENT4);
    CHECK(rval, expected, CSR_MHPMEVENT4);

    rval = read_csr(CSR_MHPMEVENT5);
    expected = mhpmevent_be;
    write_csr(CSR_MHPMEVENT5, expected);
    rval = read_csr(CSR_MHPMEVENT5);
    CHECK(rval, expected, CSR_MHPMEVENT5);

    rval = read_csr(CSR_MHPMEVENT6);
    expected = mhpmevent_fe_ic;
    write_csr(CSR_MHPMEVENT6, expected);
    rval = read_csr(CSR_MHPMEVENT6);
    CHECK(rval, expected, CSR_MHPMEVENT6);

    rval = read_csr(CSR_MHPMEVENT7);
    expected = mhpmevent_be_dc;
    write_csr(CSR_MHPMEVENT7, expected);
    rval = read_csr(CSR_MHPMEVENT7);
    CHECK(rval, expected, CSR_MHPMEVENT7);

    rval = read_csr(CSR_MHPMEVENT8);
    expected = mhpmevent_ret_simd;
    write_csr(CSR_MHPMEVENT8, expected);
    rval = read_csr(CSR_MHPMEVENT8);
    CHECK(rval, expected, CSR_MHPMEVENT8);

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
    rval = read_csr(CSR_MHPMCOUNTER3);
    if (rval == 0) {
        write_mismatch(rval, cnt, CSR_MHPMCOUNTER3);
        fail();
    }

    rval = read_csr(CSR_MHPMCOUNTER4);
    if (rval == 0) {
        write_mismatch(rval, cnt, CSR_MHPMCOUNTER4);
        fail();
    }

    rval = read_csr(CSR_MHPMCOUNTER5);
    if (rval == 0) {
        write_mismatch(rval, sum, CSR_MHPMCOUNTER5);
        fail();
    }

    rval = read_csr(CSR_MHPMCOUNTER6);
    if (rval == 0) {
        write_mismatch(rval, sum, CSR_MHPMCOUNTER6);
        fail();
    }

    rval = read_csr(CSR_MHPMCOUNTER7);
    if (rval == 0) {
        write_mismatch(rval, sum, CSR_MHPMCOUNTER7);
        fail();
    }

    rval = read_csr(CSR_MHPMCOUNTER8);
    if (rval == 0) {
        write_mismatch(rval, sum, CSR_MHPMCOUNTER8);
        fail();
    }

    pass();
}
