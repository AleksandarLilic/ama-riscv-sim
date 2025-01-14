#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#include "csr.h"
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

#ifdef CUSTOM_ISA
#define LOAD_AND_RESERVE_SCP(addr) asm volatile("scp.lcl x0, %0" : : "r"(addr))
// TODO: can reserve with rd!=0 if success/fail status is needed
#define RELEASE_SCP(addr) asm volatile("scp.rel x0, %0" : : "r"(addr))
// TODO: same as above for scp.rel
#endif

// custom data types
typedef union {
    uint64_t u64;
    uint32_t u32[2];
} read64_t;

// functions
inline __attribute__ ((always_inline))
void write_csr(const uint32_t csr_addr, uint32_t data) {
    asm volatile("csrw %[a], %[d]" : : [a] "r" (csr_addr), [d] "r" (data));
}

inline __attribute__ ((always_inline))
uint32_t read_csr(const uint32_t csr_addr) {
    uint32_t data;
    asm volatile("csrr %[d], %[a]" : [d] "=r" (data) : [a] "i" (csr_addr));
    return data;
}

inline __attribute__ ((always_inline))
void write_tohost_testnum() {
    asm volatile("csrw 0x51e, " TOSTR(TESTNUM_REG));
}

inline __attribute__ ((always_inline))
void pass() {
    asm volatile("li " TOSTR(TESTNUM_REG) ", 1");
    write_tohost_testnum();
}

inline __attribute__ ((always_inline))
void fail() {
    asm volatile("sll " TOSTR(TESTNUM_REG) ", " TOSTR(TESTNUM_REG) ", 1");
    asm volatile("or " TOSTR(TESTNUM_REG) ", " TOSTR(TESTNUM_REG) ", 1");
    write_tohost_testnum();
}

inline __attribute__ ((always_inline))
void write_mismatch(uint32_t res, uint32_t ref, uint32_t idx) {
    asm volatile("add " TOSTR(TESTNUM_REG) ", x0, %0" : : "r"(idx));
    asm volatile("add x29, x0, %0" : : "r"(res));
    asm volatile("add x30, x0, %0" : : "r"(ref));
}

// function prototypes
void send_byte_uart0(char byte);
int _write(int fd, char* ptr, int len);
int __puts_uart(char* s, int len, void *buf);
int mini_printf(const char* format, ...);
uint64_t get_cpu_time();
uint64_t get_cpu_cycles();
uint64_t get_cpu_instret();
void trap_handler(unsigned int mcause, void* mepc, void* sp);

#endif
