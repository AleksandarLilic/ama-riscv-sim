#include <stdint.h>

void write_mismatch(uint32_t res, uint32_t ref, uint8_t idx) {
    asm volatile("add x29, x0, %0"
                 :
                 : "r"(res));
    asm volatile("add x30, x0, %0"
                 :
                 : "r"(ref));
    asm volatile("add x28, x0, %0"
                 :
                 : "r"(idx+1)); // to avoid writing 0
}
