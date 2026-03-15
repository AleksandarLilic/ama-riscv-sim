#include "common.h"
#include "common_math.h"

#include "ds.h"
#include "test_arrays.h"

#ifndef LOOPS
#define LOOPS 1u
#endif

#ifndef STRIDE
#define STRIDE 1
#endif

// typical "cheap temp compensation"
// a crude correction (shift) without_idx multiplies
//   temp < 25C  -> shift 1  (x2)
//   temp < 50C  -> shift 2  (x4)
//   temp >= 50C -> shift 3  (x8)
static inline uint32_t temp_shift(int16_t temp_c) {
    if (temp_c < 25) return 1;
    if (temp_c < 50) return 2;
    return 3;
}

// "energy" feature (no sqrt): ax^2 + ay^2 + az^2
static inline uint32_t accel_energy(int16_t ax, int16_t ay, int16_t az) {
    int32_t x = ax, y = ay, z = az;
    return (uint32_t)(x*x + y*y + z*z);
}

// "bias-ish" accumulator: sum components
static inline void gyro_accum(
    int16_t gx, int16_t gy, int16_t gz, int64_t *sx, int64_t *sy, int64_t *sz)
{
    *sx += gx;
    *sy += gy;
    *sz += gz;
}

// prevent compiler opt
#define HASH_CONST 0x9e3779b97f4a7c15ULL
static inline uint64_t hash(
    uint64_t e_sum, int64_t sx, int64_t sy, int64_t sz)
{
    uint64_t p = (uint64_t)(uint32_t)sx ^
                 ((uint64_t)(uint32_t)sy << 21) ^
                 ((uint64_t)(uint32_t)sz << 42);
    return e_sum ^ (p * HASH_CONST);
}


// staged kernels
// two passes: first computes accel feature; second computes gyro sums
// models pipelines where stages consume different subsets
static uint64_t aos_staged(void) {
    uint64_t e_sum = 0;
    GLOBAL_SYMBOL("accel");
    for (int i = 0; i < (int)ARR_LEN; i += STRIDE) {
        if (imu_aos[i].flags & 1) {
            e_sum += (uint64_t)accel_energy(
                imu_aos[i].ax, imu_aos[i].ay, imu_aos[i].az);
        }
    }

    GLOBAL_SYMBOL("gyro");
    int64_t sx = 0, sy = 0, sz = 0;
    for (int i = 0; i < (int)ARR_LEN; i += STRIDE) {
        if (imu_aos[i].flags & 1) {
            gyro_accum(
                imu_aos[i].gx, imu_aos[i].gy, imu_aos[i].gz, &sx, &sy, &sz);
        }
    }

    return hash(e_sum, sx, sy, sz);
}

static uint64_t soa_staged(void) {
    uint64_t e_sum = 0;
    GLOBAL_SYMBOL("accel");
    for (int i = 0; i < (int)ARR_LEN; i += STRIDE) {
        if (imu_soa.flags[i] & 1) {
            e_sum += (uint64_t)accel_energy(
                imu_soa.ax[i], imu_soa.ay[i], imu_soa.az[i]);
        }
    }

    GLOBAL_SYMBOL("gyro");
    int64_t sx = 0, sy = 0, sz = 0;
    for (int i = 0; i < (int)ARR_LEN; i += STRIDE) {
        if (imu_soa.flags[i] & 1) {
            gyro_accum(
                imu_soa.gx[i], imu_soa.gy[i], imu_soa.gz[i], &sx, &sy, &sz);
        }
    }

    return hash(e_sum, sx, sy, sz);
}

// combined kernels
// one pass: uses accel + gyro + temp in a single loop
// models "full consume per sample" kernels
static uint64_t aos_combined(void) {
    uint64_t e_sum = 0;
    int64_t sx = 0, sy = 0, sz = 0;

    for (int i = 0; i < (int)ARR_LEN; i += STRIDE) {
        if (imu_aos[i].flags & 1) {
            uint32_t sh = temp_shift(imu_aos[i].temp_c);

            GLOBAL_SYMBOL("accel");
            e_sum += ((uint64_t)accel_energy(
                imu_aos[i].ax, imu_aos[i].ay, imu_aos[i].az)) << sh;

            GLOBAL_SYMBOL("gyro");
            gyro_accum((int16_t)(imu_aos[i].gx << sh),
                       (int16_t)(imu_aos[i].gy << sh),
                       (int16_t)(imu_aos[i].gz << sh),
                       &sx, &sy, &sz);
        }
    }

    return hash(e_sum, sx, sy, sz);
}

static uint64_t soa_combined(void) {
    uint64_t e_sum = 0;
    int64_t sx = 0, sy = 0, sz = 0;

    for (int i = 0; i < (int)ARR_LEN; i += STRIDE) {
        if (imu_soa.flags[i] & 1) {
            uint32_t sh = temp_shift(imu_soa.temp_c[i]);

            GLOBAL_SYMBOL("accel");
            e_sum += ((uint64_t)accel_energy(
                imu_soa.ax[i], imu_soa.ay[i], imu_soa.az[i])) << sh;

            GLOBAL_SYMBOL("gyro");
            gyro_accum((int16_t)(imu_soa.gx[i] << sh),
                       (int16_t)(imu_soa.gy[i] << sh),
                       (int16_t)(imu_soa.gz[i] << sh),
                       &sx, &sy, &sz);
        }
    }

    return hash(e_sum, sx, sy, sz);
}

uint64_t kernel_run(void) {
    return
    #if defined(DS_AOS)
    #ifdef ALG_STAGED
    aos_staged();
    #else
    aos_combined();
    #endif
    #elif defined(DS_SOA)
    #ifdef ALG_STAGED
    soa_staged();
    #else
    soa_combined();
    #endif
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
        PROF_START;
        volatile uint64_t r = kernel_run();
        PROF_STOP;
        printf("kernel_run=%u\n", (unsigned long long)r);
    }
    pass();
}
