#include <stdint.h>

#include "common.h"

// timer + external interrupts with WFI, targets FPGA

#ifndef HEARTBEAT_US
#define HEARTBEAT_US 10000000u
#endif

#ifndef BATCH
#define BATCH 8
#endif

static volatile uint32_t beats = 0;
static volatile uint8_t buf[BATCH];
static volatile int32_t n = 0;

// bump the compare and print a heartbeat (time, cycles, instret)
static inline void heartbeat(void) {
    CLINT->mtimecmp += HEARTBEAT_US;
    sliced64_t t;
    t = (sliced64_t)CLINT->mtime;
    uint64_t i, c;
    read_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, i);
    read_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, c);
    t.u64 = (t.u64 / 1000000);
    printf("[hb %u] t=%llu s, c=%llu, i=%llu\n", ++beats, t.u64, c, i);
}

// consume one RX byte (clears RX_VALID) and buffer it; loop the batch back
static inline void loopback_rx(void) {
    uint8_t b = (uint8_t)UART0->rx_data;
    if (b == 0xA) return; // ignore line feed
    // flip the case on letters only, others pass through
    if (b >= 'A' && b <= 'Z') b += 0x20;
    else if (b >= 'a' && b <= 'z') b -= 0x20;
    buf[n++] = b;
    if (n == BATCH) { // loop the swapped batch back, newline at the end
        for (int32_t i = 0; i < BATCH; i++) {
            while (!UART0_TX_READY);
            UART0->tx_data = buf[i];
        }
        while (!UART0_TX_READY);
        UART0->tx_data = '\n';
        n = 0;
    }
}

#if defined(INTERRUPT_WFI) || defined(INTERRUPT_IDLING)
void timer_interrupt_handler() { heartbeat(); }
void external_interrupt_handler() { loopback_rx(); }
#else // WAKEUP_WFI
static inline void service(void) {
    if (CLINT->mtime >= CLINT->mtimecmp) heartbeat();
    if (UART0_RX_VALID) loopback_rx();
}
#endif

void main(void) {
    CLINT->mtime = 0x0;
    CLINT->mtimecmp = HEARTBEAT_US;

    // enable machine timer + external int (also the WFI wake sources)
    set_csr(CSR_MIE, MIE_MTIE | MIE_MEIE);

    #if defined(INTERRUPT_WFI) || defined(INTERRUPT_IDLING)
    set_csr(CSR_MSTATUS, MSTATUS_MIE); // take traps; ISRs service the sources
    #endif
    // WAKEUP_WFI: mstatus.MIE left 0 -> WFI wakes but no trap is taken

    for (;;) {
        #ifdef INTERRUPT_IDLING
        NOP;
        #elif defined(INTERRUPT_WFI)
        WFI;
        #elif defined(WAKEUP_WFI)
        WFI;
        service();
        #endif
    }
}
