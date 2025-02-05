#ifndef MEM_MAP_H
#define MEM_MAP_H

#include <stdint.h>

typedef volatile struct __attribute__((packed, aligned(4))) {
    volatile uint32_t ctrl;
    volatile uint32_t rx_data;
    volatile uint32_t tx_data;
} uart_t;

typedef volatile struct __attribute__((packed, aligned(8))) {
    volatile uint64_t msip;
    volatile uint64_t mtimecmp;
    volatile uint64_t mtime;
} clint_t;

#define UART0 ((uart_t*) (0x20000))
#define CLINT ((clint_t*) (0x20000 + 0x20))

#endif
