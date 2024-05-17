#include <stdint.h>
#include "common.h"
#include "mem_map.h"

#define OFFSET 0x20
void main(void) {
    for (char i = 0; i < 10; i++) {
        while (!UART0_RX_VALID);
        char received_byte = UART0->rx_data;
        while (!UART0_TX_READY);
        // convert to uppercase and send back
        UART0->tx_data = received_byte - OFFSET;
    }
    asm volatile("ecall");
}
