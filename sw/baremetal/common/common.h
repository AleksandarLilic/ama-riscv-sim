#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#include "mem_map.h"
#include "mini-printf.h"

#ifndef FORCE_NEWLIB_PRINTF
// not exactly the same as libc printf, but close enough for the purpose
#define printf mini_printf
#endif

#define STRINGIFY(x) #x
#define TOSTR(x) STRINGIFY(x)
#define TESTNUM_REG x28

// logging features
#define LOG_START asm volatile ("slti x0, x0, 0x10")
#define LOG_STOP asm volatile ("slti x0, x0, 0x11")

void test_end();
void write_mismatch(uint32_t res, uint32_t ref, uint8_t idx);
void write_csr_status();
void pass();
void fail();
void send_byte_uart0(char byte);
int _write(int fd, char *ptr, int len);
int __puts_uart(char *s, int len, void *buf);
int mini_printf(const char* format, ...);
uint32_t time_us();
uint32_t clock_ticks();
void _putchar(char character); // tiny printf implementation

#endif
