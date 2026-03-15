#include "common.h"
#include "common_math.h"

#include "test_arrays.h"

typedef struct {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t temp_c; // integer degrees C
    uint8_t flags; // bit0: VALID
    uint8_t _pad;
} imu_t;

typedef struct {
    int16_t ax[ARR_LEN], ay[ARR_LEN], az[ARR_LEN];
    int16_t gx[ARR_LEN], gy[ARR_LEN], gz[ARR_LEN];
    int16_t temp_c[ARR_LEN];
    uint8_t flags[ARR_LEN];
} imu_arr_t;

imu_t imu_aos[ARR_LEN];
imu_arr_t imu_soa;

void load_aos(){
    for (size_t i = 0; i < ARR_LEN; i++)
    {
        imu_aos[i].ax = ax[i];
        imu_aos[i].ay = ay[i];
        imu_aos[i].az = az[i];
        imu_aos[i].gx = gx[i];
        imu_aos[i].gy = gy[i];
        imu_aos[i].gz = gz[i];
        imu_aos[i].temp_c = temp[i];
        imu_aos[i].flags = flags[i];
    }
}

void load_soa(){
    for (size_t i = 0; i < ARR_LEN; i++)
    {
        imu_soa.ax[i] = ax[i];
        imu_soa.ay[i] = ay[i];
        imu_soa.az[i] = az[i];
        imu_soa.gx[i] = gx[i];
        imu_soa.gy[i] = gy[i];
        imu_soa.gz[i] = gz[i];
        imu_soa.temp_c[i] = temp[i];
        imu_soa.flags[i] = flags[i];
    }
}
