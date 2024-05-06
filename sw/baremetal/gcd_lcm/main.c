#include <stdint.h>
#include "common.h"
#include "func.h"

#define LOOP_COUNT 1u

#define INPUT_A 10440125
#define INPUT_B 157216
#define EXPECTED_GCD 4913
#define EXPECTED_LCM 334084000

void main() {
    volatile uint32_t a = INPUT_A;
    volatile uint32_t b = INPUT_B;
    
    for (uint32_t i = 0; i < LOOP_COUNT; i++) {
        uint64_t result = lcm(a, b);
        if (result != EXPECTED_LCM){
            write_mismatch(result, EXPECTED_LCM, 1);
            fail();
        }
    }
    pass();
}
