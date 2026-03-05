#include <stdint.h>
#include "common.h"
#include "common_math.h"

#define CHECK(val, expected, csr_num) \
    if (val != expected){ \
        write_mismatch(val, expected, csr_num); \
        fail(); \
    }

#define CHECK_CSR(csr_addr, val) \
    write_csr(CSR_MSCRATCH, 0); \
    read_csr(CSR_MSCRATCH, rval); \
    CHECK(rval, 0, CSR_MSCRATCH);

#define TEST_CSR(csr_addr, ev) \
    write_csr(CSR_MHPMEVENT4, ev); \
    read_csr(CSR_MHPMEVENT4, rval); \
    CHECK(rval, ev, CSR_MHPMEVENT4);

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

    // check all events
    uint32_t rval;
    uint32_t ev;
    ev = mhpmevent_bad_spec;
    TEST_CSR(CSR_MHPMEVENT3, ev);
    ev = mhpmevent_fe;
    TEST_CSR(CSR_MHPMEVENT4, ev);
    ev = mhpmevent_be;
    TEST_CSR(CSR_MHPMEVENT5, ev);
    ev = mhpmevent_fe_ic;
    TEST_CSR(CSR_MHPMEVENT6, ev);
    ev = mhpmevent_be_dc;
    TEST_CSR(CSR_MHPMEVENT7, ev);
    ev = mhpmevent_ret_simd;
    TEST_CSR(CSR_MHPMEVENT8, ev);

    ev = mhpmevent_ret_ctrl_flow;
    TEST_CSR(CSR_MHPMEVENT3, ev);
    ev = mhpmevent_ret_ctrl_flow_j;
    TEST_CSR(CSR_MHPMEVENT4, ev);
    ev = mhpmevent_ret_ctrl_flow_jr;
    TEST_CSR(CSR_MHPMEVENT5, ev);
    ev = mhpmevent_ret_ctrl_flow_br;
    TEST_CSR(CSR_MHPMEVENT6, ev);
    ev = mhpmevent_ret_mem;
    TEST_CSR(CSR_MHPMEVENT7, ev);
    ev = mhpmevent_ret_mem_load;
    TEST_CSR(CSR_MHPMEVENT8, ev);
    ev = mhpmevent_ret_mem_store;
    TEST_CSR(CSR_MHPMEVENT3, ev);

    ev = mhpmevent_ret_simd_arith;
    TEST_CSR(CSR_MHPMEVENT4, ev);
    ev = mhpmevent_ret_simd_data_fmt;
    TEST_CSR(CSR_MHPMEVENT5, ev);

    ev = mhpmevent_core_stall_simd;
    TEST_CSR(CSR_MHPMEVENT6, ev);
    ev = mhpmevent_core_stall_load;
    TEST_CSR(CSR_MHPMEVENT7, ev);

    ev = mhpmevent_l1i_access;
    TEST_CSR(CSR_MHPMEVENT8, ev);
    ev = mhpmevent_l1i_miss;
    TEST_CSR(CSR_MHPMEVENT3, ev);
    ev = mhpmevent_l1i_spec_miss;
    TEST_CSR(CSR_MHPMEVENT4, ev);
    ev = mhpmevent_l1i_spec_miss_bad;
    TEST_CSR(CSR_MHPMEVENT5, ev);
    ev = mhpmevent_l1i_spec_miss_good;
    TEST_CSR(CSR_MHPMEVENT6, ev);

    ev = mhpmevent_l1d_access;
    TEST_CSR(CSR_MHPMEVENT7, ev);
    ev = mhpmevent_l1d_miss;
    TEST_CSR(CSR_MHPMEVENT8, ev);
    ev = mhpmevent_l1d_writeback;
    TEST_CSR(CSR_MHPMEVENT3, ev);

    pass();
}
