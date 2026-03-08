#include <stdint.h>
#include "common.h"
#include "common_math.h"

#define CHECK(val, expected, csr_num) \
    if (val != expected){ \
        write_mismatch(val, expected, csr_num); \
        fail(); \
    }

#define TEST_CSR(csr_addr, event) \
    write_csr(csr_addr, event); \
    read_csr(csr_addr, rval); \
    CHECK(rval, event, csr_addr);

void main() {
    uint32_t rval;
    TEST_CSR(CSR_MHPMEVENT3, mhpmevent_bad_spec);
    TEST_CSR(CSR_MHPMEVENT4, mhpmevent_stall_be);
    TEST_CSR(CSR_MHPMEVENT5, mhpmevent_stall_l1d);
    TEST_CSR(CSR_MHPMEVENT6, mhpmevent_stall_l1d_r);
    TEST_CSR(CSR_MHPMEVENT7, mhpmevent_stall_l1d_w);
    TEST_CSR(CSR_MHPMEVENT8, mhpmevent_stall_fe);
    TEST_CSR(CSR_MHPMEVENT3, mhpmevent_stall_l1i);
    TEST_CSR(CSR_MHPMEVENT4, mhpmevent_stall_simd);
    TEST_CSR(CSR_MHPMEVENT5, mhpmevent_stall_load);
    TEST_CSR(CSR_MHPMEVENT6, mhpmevent_ret_ctrl_flow);
    TEST_CSR(CSR_MHPMEVENT7, mhpmevent_ret_ctrl_flow_j);
    TEST_CSR(CSR_MHPMEVENT8, mhpmevent_ret_ctrl_flow_jr);
    TEST_CSR(CSR_MHPMEVENT3, mhpmevent_ret_ctrl_flow_br);
    TEST_CSR(CSR_MHPMEVENT4, mhpmevent_ret_mem);
    TEST_CSR(CSR_MHPMEVENT5, mhpmevent_ret_mem_load);
    TEST_CSR(CSR_MHPMEVENT6, mhpmevent_ret_mem_store);
    TEST_CSR(CSR_MHPMEVENT7, mhpmevent_ret_simd);
    TEST_CSR(CSR_MHPMEVENT8, mhpmevent_ret_simd_arith);
    TEST_CSR(CSR_MHPMEVENT3, mhpmevent_ret_simd_data_fmt);
    TEST_CSR(CSR_MHPMEVENT4, mhpmevent_l1i_ref);
    TEST_CSR(CSR_MHPMEVENT5, mhpmevent_l1i_miss);
    TEST_CSR(CSR_MHPMEVENT6, mhpmevent_l1i_spec_miss);
    TEST_CSR(CSR_MHPMEVENT7, mhpmevent_l1i_spec_miss_bad);
    TEST_CSR(CSR_MHPMEVENT8, mhpmevent_l1i_spec_miss_good);
    TEST_CSR(CSR_MHPMEVENT3, mhpmevent_l1d_ref);
    TEST_CSR(CSR_MHPMEVENT4, mhpmevent_l1d_ref_r);
    TEST_CSR(CSR_MHPMEVENT5, mhpmevent_l1d_ref_w);
    TEST_CSR(CSR_MHPMEVENT6, mhpmevent_l1d_miss);
    TEST_CSR(CSR_MHPMEVENT7, mhpmevent_l1d_miss_r);
    TEST_CSR(CSR_MHPMEVENT8, mhpmevent_l1d_miss_w);
    TEST_CSR(CSR_MHPMEVENT3, mhpmevent_l1d_writeback);

    pass();
}
