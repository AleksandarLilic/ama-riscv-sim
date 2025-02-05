#include <stdint.h>

#include "common.h"
#include "mini-printf.h"

void timer_interrupt_handler() {
    CLINT->mtimecmp += 1000;
}

void main(void) {
    CLINT->mtime = 0x0;
    CLINT->mtimecmp = 200;
    set_csr(CSR_MIE, MIE_MTIE); // Enable machine timer interrupt
    set_csr(CSR_MSTATUS, MSTATUS_MIE); // Enable machine interrupts globally
    while (!UART0_TX_READY);
    sliced64_t mtime;
    mtime = (sliced64_t)CLINT->mtime;
    //CLINT->mtime = 0x100;
    printf("%u us\n", mtime.u32[0]);
    mtime = (sliced64_t)CLINT->mtime;
    printf("after wr: %u us\n", mtime.u32[0]);
    pass();
}
