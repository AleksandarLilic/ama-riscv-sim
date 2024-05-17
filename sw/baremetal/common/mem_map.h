#ifndef MEM_MAP_H
#define MEM_MAP_H

#include <stdint.h>

typedef struct {
    volatile uint32_t ctrl;
    volatile uint32_t rx_data;
    volatile uint32_t tx_data;
} UART_t;

extern uint32_t __uart0_base;
#define UART0 ((UART_t*) &__uart0_base)
#define UART0_TX_READY (UART0->ctrl & 0b01)
#define UART0_RX_VALID (UART0->ctrl & 0b10)

#endif
