#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

#include "csr.h"
#include "mem_map.h"
#include "mini-printf.h"

#define UART0_TX_READY (UART0->ctrl & 0x1)
#define UART0_RX_VALID (UART0->ctrl & 0x2)

#ifndef FORCE_NEWLIB_PRINTF
// not exactly the same as libc printf, but close enough for the purpose
#define printf mini_printf
#endif

#define STRINGIFY(x) #x
#define TOSTR(x) STRINGIFY(x)
#define TESTNUM_REG x28

// profiling features
#ifdef NO_PROF
#define PROF_START
#define PROF_STOP
#else
#define PROF_START asm volatile ("slti x0, x0, 0x10")
#define PROF_STOP asm volatile ("slti x0, x0, 0x11")
#endif

// add symbol for callstack
#define GLOBAL_SYMBOL(sym)  \
asm(                        \
    ".globl " #sym "\n"     \
    #sym ":"                \
)

#ifdef CUSTOM_ISA
#define SCP_LCL(addr) asm volatile("scp.lcl x0, %0" : : "r"(addr))
// TODO: can reserve with rd!=0 if success/fail status is needed
#define SCP_REL(addr) asm volatile("scp.rel x0, %0" : : "r"(addr))
// TODO: same as above for scp.rel
#endif

// HW specific
#define CACHE_LINE_SIZE 64 // bytes
#define CACHE_BYTE_ADDR_BITS (__builtin_ctz(CACHE_LINE_SIZE)) // 6
#define CACHE_BYTE_ADDR_MASK (CACHE_LINE_SIZE - 1) // 0x3F, bottom 6 bits

// custom data types
typedef union {
    uint64_t u64;
    uint32_t u32[2];
    uint16_t u16[4];
    uint8_t u8[8];
} sliced64_t;

typedef union {
    uint32_t u32;
    uint16_t u16[2];
    uint8_t u8[4];
} sliced32_t;

// mhpm events
enum mhpmevent_t {
    mhpmevent_bad_spec = (1 << 0),
    mhpmevent_be = (1 << 1),
    mhpmevent_be_dc = (1 << 2),
    mhpmevent_fe = (1 << 3),
    mhpmevent_fe_ic = (1 << 4),
    mhpmevent_ret_simd = (1 << 5),
};

// functions
inline __attribute__ ((always_inline))
void write_csr(const uint32_t csr_addr, uint32_t data) {
    asm volatile("csrw %[a], %[d]" : : [a] "i" (csr_addr), [d] "r" (data));
}

inline __attribute__ ((always_inline))
uint32_t read_csr(const uint32_t csr_addr) {
    uint32_t data;
    asm volatile("csrr %[d], %[a]" : [d] "=r" (data) : [a] "i" (csr_addr));
    return data;
}

inline __attribute__ ((always_inline))
void set_csr(const uint32_t csr_addr, uint32_t bits) {
    asm volatile("csrs %[a], %[d]" : : [a] "i" (csr_addr), [d] "r" (bits));
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
void set_cpu_cycles(uint64_t value);
void set_cpu_instret(uint64_t value);
void trap_handler(unsigned int mcause, void* mepc, void* sp);
void timer_interrupt_handler();

#endif
