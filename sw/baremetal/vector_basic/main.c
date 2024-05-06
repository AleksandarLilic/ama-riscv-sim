#include "common.h"

#if defined(OP_ADD)
#include "test_arrays_add.h"
#elif defined(OP_SUB)
#include "test_arrays_sub.h"
#elif defined(OP_MUL)
#include "test_arrays_mul.h"
#elif defined(OP_DIV)
#include "test_arrays_div.h"
#else
_Static_assert(0, "No operation defined");
#endif

#define LOOP_COUNT 1u

void set_c() {
    for (uint8_t i = 0; i < ARR_LEN; i++)
        c[i] = 0;
}

void main(void) {
    for (uint32_t i = 0; i < LOOP_COUNT; i++) {
        set_c();

        asm(".global compute");
        asm("compute:");
        for (uint8_t k = 0; k < ARR_LEN; k++) {
            c[k] = a[k] OP b[k];
        }

        asm(".global check");
        asm("check:");
        for (uint8_t j = 0; j < ARR_LEN; j++) {
            if (c[j] != ref[j]) {
                write_mismatch(c[j], ref[j], j+1); // +1 to avoid writing 0
                fail();
            }
        }
    }
    pass();
}
