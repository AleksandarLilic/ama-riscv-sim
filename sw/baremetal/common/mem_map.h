#ifndef MEM_MAP_H
#define MEM_MAP_H

#include <stdint.h>

typedef volatile struct __attribute__((packed, aligned(4))) {
    volatile uint32_t ctrl;
    volatile uint32_t rx_data;
    volatile uint32_t tx_data;
} uart_t;

extern uint32_t __uart0_base[3]; // symbol defined in linker script
#define UART0_BASE ((uintptr_t) &__uart0_base)
#define UART0 ((uart_t*) (UART0_BASE))
//#define UART0 ((uart_t*) (0x18000)) // absolute addr w/o linker script symbol

#endif
