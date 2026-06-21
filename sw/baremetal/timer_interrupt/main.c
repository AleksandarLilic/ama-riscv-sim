#include <stdint.h>

#include "common.h"

void timer_interrupt_handler() {
    CLINT->mtimecmp += 100;
}

void main(void) {
    CLINT->mtime = 0x0;
    CLINT->mtimecmp = 10;
    set_csr(CSR_MIE, MIE_MTIE); // Enable machine timer interrupt
    set_csr(CSR_MSTATUS, MSTATUS_MIE); // Enable machine interrupts globally
    while (!UART0_TX_READY);
    volatile int32_t c = 30;
    while (c--) {};
    sliced64_t mtime;
    mtime = (sliced64_t)CLINT->mtime;
    //CLINT->mtime = 0x100;
    printf("%u us\n", mtime.u32[0]);
    mtime = (sliced64_t)CLINT->mtime;
    printf("after wr: %u us\n", mtime.u32[0]);
    pass();
}
