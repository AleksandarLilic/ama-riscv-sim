#include <stdint.h>

#include "common.h"

// run with: --uart_in "A" (UART_IN=1 build); the byte arrives externally,
// paced at the baud-rate instruction stride -> RX_VALID -> mip.MEIP -> trap
#define EXPECTED_RX 'A'

static volatile int32_t serviced = 0;
static volatile uint8_t rx_byte = 0;

void external_interrupt_handler() {
    rx_byte = (uint8_t)UART0->rx_data;
    serviced++;
}

void main(void) {
    set_csr(CSR_MIE, MIE_MEIE); // enable machine external interrupt
    set_csr(CSR_MSTATUS, MSTATUS_MIE); // enable machine interrupts globally

    // spin until the handler runs
    // (bounded so a missing --uart_in fails, not hangs)
    for (volatile uint32_t i = 0; (i < 1000000) && !serviced; i++);

    if (serviced != 1) {
        write_mismatch(serviced, 1, 1);
        fail();
    }

    if (rx_byte != EXPECTED_RX) {
        write_mismatch(rx_byte, EXPECTED_RX, 2);
        fail();
    }

    pass();
}
