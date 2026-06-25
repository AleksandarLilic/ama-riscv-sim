#include <stdint.h>

#include "common.h"

void timer_interrupt_handler() {
    CLINT->mtimecmp += 100;
}

void main(void) {
    const uint32_t mtimecmp_nx = 10;
    CLINT->mtime = 0x0;
    CLINT->mtimecmp = mtimecmp_nx;
    set_csr(CSR_MIE, MIE_MTIE); // Enable machine timer interrupt
    #ifndef TYPE_WFI_NO_GLOBAL
    set_csr(CSR_MSTATUS, MSTATUS_MIE); // Enable machine interrupts globally
    #endif

    sliced64_t mtime;
    mtime = (sliced64_t)CLINT->mtime;

    // choose how to wait
    #ifdef TYPE_IDLING
    // wait similar amount of time (mtimecmp_nx ticks) in busy idle
    const uint32_t ticks_per_mtime_inc = 100;
    const uint32_t inst_in_loop = 3; // expected: nop, decrement, compare
    const uint32_t passed = 18; // already passed from start, approx, observed
    int32_t c = (mtimecmp_nx * ticks_per_mtime_inc / inst_in_loop - passed);
    while (c--) { NOP; };
    #endif

    #if defined(TYPE_WFI) || defined(TYPE_WFI_NO_GLOBAL)
    // wfi works without global (i.e. mstatus.MIE) enabled, if mie.MTIE is set
    WFI;
    #endif

    printf("before: %u us\n", mtime.u32[0]);
    mtime = (sliced64_t)CLINT->mtime;
    printf("after: %u us\n", mtime.u32[0]);
    pass();
}
