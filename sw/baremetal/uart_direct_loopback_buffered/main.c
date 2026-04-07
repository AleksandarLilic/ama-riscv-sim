#include <stdint.h>
#include "common.h"
#include "mem_map.h"

#define OFFSET 0x20
#define ITERS 12

int32_t buf [ITERS];

void main(void) {
    // buffer incoming characters
    for (int8_t i = 0; i < ITERS; i++) {
        while (!UART0_RX_VALID);
        int8_t recv_b = UART0->rx_data;
        // if lowercase letter, convert to upper
        if (recv_b >= 97 && recv_b <= 122) recv_b -= OFFSET;
        buf[i] = recv_b;
    }

    // read all back
    for (int8_t i = 0; i < ITERS; i++) {
        while (!UART0_TX_READY);
        UART0->tx_data = buf[i];
    }

    // newline at the end
    while (!UART0_TX_READY);
    UART0->tx_data = '\n';
    pass();
}
