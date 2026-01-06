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

#define ICACHE_SETS 32
#define ICACHE_WAYS 2
#define DCACHE_SETS 16
#define DCACHE_WAYS 4

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

#ifdef CUSTOM_ISA
// simd data types
typedef struct { uint32_t v; } uint16x2_t;
typedef struct { uint32_t v; } int16x2_t;
typedef struct { uint32_t v; } uint8x4_t;
typedef struct { uint32_t v; } int8x4_t;
typedef struct { uint32_t v; } uint4x8_t;
typedef struct { uint32_t v; } int4x8_t;
typedef struct { uint32_t v; } uint2x16_t;
typedef struct { uint32_t v; } int2x16_t;

// simd wide data types
typedef union { uint64_t d; struct { uint32_t lo, hi; } w; } uint32x2_t;
typedef union { uint64_t d; struct { uint32_t lo, hi; } w; } int32x2_t;
typedef union { uint64_t d; struct { uint16x2_t lo, hi; } w; } uint16x4_t;
typedef union { uint64_t d; struct { int16x2_t lo, hi; } w; } int16x4_t;
typedef union { uint64_t d; struct { uint8x4_t lo, hi; } w; } uint8x8_t;
typedef union { uint64_t d; struct { int8x4_t lo, hi; } w; } int8x8_t;
typedef union { uint64_t d; struct { uint4x8_t lo, hi; } w; } uint4x16_t;
typedef union { uint64_t d; struct { int4x8_t lo, hi; } w; } int4x16_t;
typedef union { uint64_t d; struct { uint2x16_t lo, hi; } w; } uint2x32_t;
typedef union { uint64_t d; struct { int2x16_t lo, hi; } w; } int2x32_t;
#endif

// mhpm events
enum mhpmevent_t {
    mhpmevent_bad_spec = (1 << 0),
    mhpmevent_be = (1 << 1),
    mhpmevent_be_dc = (1 << 2),
    mhpmevent_fe = (1 << 3),
    mhpmevent_fe_ic = (1 << 4),
    mhpmevent_ret_simd = (1 << 5),
};

typedef struct {
    uint64_t cycles;
    uint64_t bad_spec;
    uint64_t be;
    uint64_t be_dc;
    uint64_t be_core;
    uint64_t fe;
    uint64_t fe_ic;
    uint64_t fe_core;
    uint64_t ret_simd;
    uint64_t ret_int;
    uint64_t ret;
} perf_event_cnt_t;

// functions
#define read_csr(CSR, dest) \
    do { \
        asm volatile ("csrr %0, " TOSTR(CSR) : "=r"(dest)); \
    } while (0)

#define write_csr(CSR, val) \
    do { \
        asm volatile ("csrw " TOSTR(CSR) ", %0" :: "r"(val)); \
    } while (0)

#define set_csr(CSR, bits) \
    do { \
        asm volatile ("csrs " TOSTR(CSR) ", %0" :: "r"(bits)); \
    } while (0)

#define clear_csr(CSR, bits) \
    do { \
        asm volatile ("csrc " TOSTR(CSR) ", %0" :: "r"(bits)); \
    } while (0)


#define read_csr_wide(LO, HI, dest64) \
    do { \
        uint32_t __lo, __hi1, __hi2; \
        do { \
            read_csr(HI, __hi1); \
            read_csr(LO, __lo); \
            read_csr(HI, __hi2); \
        } while (__hi1 != __hi2); \
        (dest64) = (((uint64_t)__hi1 << 32) | __lo); \
    } while (0)

#define write_csr_wide(LO, HI, val64) \
    do { \
        uint64_t __v = (val64); \
        write_csr(HI, (uint32_t)(__v >> 32)); \
        write_csr(LO, (uint32_t)__v); \
    } while (0)

#define write_tohost_testnum() \
    asm volatile("csrw 0x51e, " TOSTR(TESTNUM_REG))

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

void set_up_perf_counters();
void save_perf_counters(perf_event_cnt_t* pe);
void print_perf_counters(const perf_event_cnt_t* pe);

void trap_handler(unsigned int mcause, void* mepc, void* sp);
void timer_interrupt_handler();

#endif
