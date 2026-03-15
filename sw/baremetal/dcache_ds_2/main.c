#include "common.h"
#include "common_math.h"

#include "ds.h"
#include "test_arrays.h"

#ifndef LOOPS
#define LOOPS 1u
#endif

#ifndef MA_W
#define MA_W 16
#endif

#ifndef MA_W_LOG2
#define MA_W_LOG2 4
#endif

#if (MA_W != (1u << MA_W_LOG2))
#error "MA_W must be power-of-2 and match MA_W_LOG2 (MA_W == 1<<MA_W_LOG2)"
#endif

static inline uint8_t is_valid(uint8_t flags) {
    return (uint8_t)(flags & 1);
}

static inline int32_t valid_temp(int16_t temp_c, uint8_t flags) {
    return is_valid(flags) ? (int32_t)temp_c : 0;
}

uint64_t aos_temp_ma() {
    int32_t sum = 0; // sum of (valid?temp:0) over the current MA_W window
    uint64_t acc = 0; // checksum of produced averages

    // prime initial window [0 .. MA_W-1] (or up to ARR_LEN if shorter)
    size_t w_end = (ARR_LEN < (size_t)MA_W) ? ARR_LEN : (size_t)MA_W;
    for (size_t i = 0; i < w_end; i++) {
        sum += valid_temp(imu_aos[i].temp_c, imu_aos[i].flags);
    }

    // for i, window conceptually includes indices [i-(MA_W-1) .. i]
    // removes out_idx=(i+1-MA_W) once i+1>=MA_W, and adds in_idx=(i+MA_W)
    for (size_t i = 0; i < ARR_LEN; i++) {
        int32_t avg = (sum >> MA_W_LOG2);
        acc += (uint32_t)avg;

        if ((i + 1) >= (size_t)MA_W) { // starts once enough samples passed
            size_t out_idx = ((i + 1) - (size_t)MA_W);
            sum -= valid_temp(imu_aos[out_idx].temp_c, imu_aos[out_idx].flags);
        }

        size_t in_idx = (i + (size_t)MA_W);
        if (in_idx < ARR_LEN) { // as long as there are new samples
            sum += valid_temp(imu_aos[in_idx].temp_c, imu_aos[in_idx].flags);
        }
    }

    return acc;
}

uint64_t soa_temp_ma() {
    int32_t  sum = 0;
    uint64_t acc = 0;

    size_t w_end = (ARR_LEN < (size_t)MA_W) ? ARR_LEN : (size_t)MA_W;
    for (size_t i = 0; i < w_end; i++) {
        sum += valid_temp(imu_soa.temp_c[i], imu_soa.flags[i]);
    }

    for (size_t i = 0; i < ARR_LEN; i++) {
        int32_t avg = (sum >> MA_W_LOG2);
        acc += (uint32_t)avg;

        if (i + 1 >= (size_t)MA_W) {
            size_t out_idx = ((i + 1) - (size_t)MA_W);
            sum -= valid_temp(imu_soa.temp_c[out_idx],imu_soa.flags[out_idx]);
        }

        size_t in_idx = (i + (size_t)MA_W);
        if (in_idx < ARR_LEN) {
            sum += valid_temp(imu_soa.temp_c[in_idx],imu_soa.flags[in_idx]);
        }
    }

    return acc;
}

uint64_t kernel_run(void) {
    return
    #if defined(DS_AOS)
    aos_temp_ma();
    #elif defined(DS_SOA)
    soa_temp_ma();
    #endif
}

int main(void) {
    GLOBAL_SYMBOL("load");
    #if defined(DS_AOS)
    load_aos();
    #elif defined(DS_SOA)
    load_soa();
    #endif
    for (uint32_t i = 0; i < LOOPS; i++) {
        GLOBAL_SYMBOL("kernel");
        PROF_START;
        volatile uint64_t r = kernel_run();
        PROF_STOP;
        printf("kernel_run=%u\n", (unsigned long long)r);
    }
    pass();
}
