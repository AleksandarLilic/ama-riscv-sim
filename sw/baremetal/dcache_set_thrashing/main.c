#include "common.h"

// dcache assumptions: 64 bytes per line, 8 cache lines, LRU replacement
// any associativity will exhibit thrashing, fully associative will be the worst

#define ARR_LEN 16

#ifdef CACHE_ALIGNED
#define ALIGN __attribute__((aligned(64)))
#else
#define ALIGN
#endif

volatile int32_t data0[ARR_LEN] ALIGN = {199340, 43567, 173685, -144192, 176963, -109829, 95939, 97639, 41993, -139565, -175851, 186098, 112420, -50601, -50106, 48600 };
volatile int32_t data1[ARR_LEN] ALIGN = {-91560, 52620, -181958, -245055, 163302, -112089, 108631, 247982, -41384, 101201, -6491, -179687, 224077, 235592, 137993, 169876 };
volatile int32_t data2[ARR_LEN] ALIGN = {67699, 70608, 168691, -254267, 83966, 132943, -189009, 133568, 179026, 155747, 108504, -230223, -32013, 131869, -212333, -62317 };
volatile int32_t data3[ARR_LEN] ALIGN = {211854, -40537, -140000, 212929, -78583, -177479, 119423, 41504, 252447, 49866, 215284, 123031, -136541, -34306, 11723, 174962 };
volatile int32_t data4[ARR_LEN] ALIGN = {148663, 251932, 58146, 108672, 223360, -71772, 89141, -190587, -111066, -127512, 221940, -200431, -165553, -216700, -14743, 257066 };
volatile int32_t data5[ARR_LEN] ALIGN = {-215622, -148961, -175240, -241407, 259393, -102425, -69719, 92217, -181981, 12134, 191607, -214261, -24146, 253010, -176037, 63360 };
volatile int32_t data6[ARR_LEN] ALIGN = {256398, -79261, 9781, 150412, -234887, 147370, -249772, 248011, 135492, 111366, 236484, 82991, -200065, -51212, -27517, -3124 };
volatile int32_t data7[ARR_LEN] ALIGN = {-124060, 9396, 243688, 3918, -252785, 116372, -86813, 154554, -115945, 240335, -160883, 37237, 79701, 8752, 259377, -154043 };
volatile int32_t data8[ARR_LEN] ALIGN = {-51031, 202403, 50624, -41889, 40133, -168354, -24320, -75919, -29006, 62756, -137310, -171216, -180387, -177789, -162206, 179754 };

int32_t loads() {
    int32_t sink = 0;
    for (int i = 0; i < ARR_LEN; ++i) {
        sink += data0[i];
        sink += data1[i];
        sink += data2[i];
        sink += data3[i];
        sink += data4[i];
        sink += data5[i];
        sink += data6[i];
        sink += data7[i];
        sink += data8[i];
    }
}

void main() {
    volatile int32_t res = loads();
    pass();
}
