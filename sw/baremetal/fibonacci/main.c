#include <stdint.h>
#include "common.h"

#define LOOP_COUNT 1u

//#define EXPECTED_FIB_RESULT 55 // for n = 10
//#define EXPECTED_FIB_RESULT 610 // for n = 15
#define EXPECTED_FIB_RESULT 2584 // for n = 18
//#define EXPECTED_FIB_RESULT 6765 // for n = 20
//#define EXPECTED_FIB_RESULT 832040 // for n = 30
//#define EXPECTED_FIB_RESULT 102334155 // for n = 40

uint32_t fib(uint32_t n) {
    if (n <= 2)
        return 1;
    else
        return fib(n-1) + fib(n-2);
}

void main() {
    volatile uint32_t n = 18;
    
    for (uint32_t i = 0; i < LOOP_COUNT; i++) {
        uint32_t result = fib(n);
        
        if (result != EXPECTED_FIB_RESULT){
            write_mismatch(result, EXPECTED_FIB_RESULT, 1);
            fail();
        }
    }
    pass();
}
