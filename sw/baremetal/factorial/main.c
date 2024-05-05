#include <stdint.h>

#define LOOP_COUNT 1u

void fail();
void pass();

//#define INPUT 10
//#define EXPECTED_FACT_RESULT 3628800

//#define INPUT 16
//#define EXPECTED_FACT_RESULT 2004189184

//#define INPUT 17
//#define EXPECTED_FACT_RESULT 355687428096000ULL

#define INPUT 20
#define EXPECTED_FACT_RESULT 2432902008176640000ULL

uint64_t factorial(uint32_t n) {
    if (n == 0)
        return 1;
    return n * factorial(n-1);
}

void main() {
    volatile uint32_t n = INPUT;
    
    for (uint32_t i = 0; i < LOOP_COUNT; i++) {
        uint64_t result = factorial(n);
        
        asm volatile("add x29, x0, %0"
                    :
                    : "r"(result>>32));
        asm volatile("add x30, x0, %0"
                    :
                    : "r"(result & 0xFFFFFFFF));
        if (result != EXPECTED_FACT_RESULT){
            asm volatile("addi x28, x0, 1"
                        :
                        : );
            fail();
        }
    }
        
    pass();
}
