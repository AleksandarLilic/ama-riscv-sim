#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include "mem_map.h"

#define STRINGIFY(x) #x
#define TOSTR(x) STRINGIFY(x)
#define TESTNUM_REG x28

void test_end();
void write_mismatch(uint32_t res, uint32_t ref, uint8_t idx);
void write_csr_status();
void pass();
void fail();
int write_uart0(int file, char *ptr, int len);
uint32_t time_us();
uint32_t clock_ticks();
void _putchar(char character); // tiny printf implementation

#endif
