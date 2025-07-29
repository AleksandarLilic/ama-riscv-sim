#ifndef MEM_MAP_H
#define MEM_MAP_H

#include <stdint.h>

#define BASE_ADDR 0x40000
//#define RAM_SIZE 0x8000 // 32K
#define RAM_SIZE 0x10000 // 64K

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

#define UART0 ((uart_t*) (BASE_ADDR + RAM_SIZE))
#define CLINT ((clint_t*) (BASE_ADDR + RAM_SIZE + 0x20))

#endif
