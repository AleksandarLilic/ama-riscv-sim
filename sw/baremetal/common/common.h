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

#define NOP asm volatile ("nop")

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

#ifdef __riscv_xsimd
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

#ifdef __riscv_xsimd
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
// can't enum beacuse C, switch back to enums with `-std=gnu23`
static const uint32_t mhpmevent_bad_spec = (1u << 0); // tda
static const uint32_t mhpmevent_stall_be = (1u << 1); // tda
static const uint32_t mhpmevent_stall_l1d = (1u << 2); // tda
static const uint32_t mhpmevent_stall_l1d_r = (1u << 3);
static const uint32_t mhpmevent_stall_l1d_w = (1u << 4);
static const uint32_t mhpmevent_stall_fe = (1u << 5); // tda
static const uint32_t mhpmevent_stall_l1i = (1u << 6); // tda
static const uint32_t mhpmevent_stall_simd = (1u << 7);
static const uint32_t mhpmevent_stall_load = (1u << 8);
static const uint32_t mhpmevent_ret_ctrl_flow = (1u <<  9);
static const uint32_t mhpmevent_ret_ctrl_flow_j = (1u << 10); // direct unconditional branch
static const uint32_t mhpmevent_ret_ctrl_flow_jr = (1u << 11); // indirect unconditional branch
static const uint32_t mhpmevent_ret_ctrl_flow_br = (1u << 12); // direct conditional branch
static const uint32_t mhpmevent_ret_mem = (1u << 13);
static const uint32_t mhpmevent_ret_mem_load = (1u << 14);
static const uint32_t mhpmevent_ret_mem_store = (1u << 15);
static const uint32_t mhpmevent_ret_simd = (1u << 16); // tda
static const uint32_t mhpmevent_ret_simd_arith = (1u << 17);
static const uint32_t mhpmevent_ret_simd_data_fmt = (1u << 18);
static const uint32_t mhpmevent_bp_miss = (1u << 19);
static const uint32_t mhpmevent_l1i_ref = (1u << 20);
static const uint32_t mhpmevent_l1i_miss = (1u << 21);
static const uint32_t mhpmevent_l1i_spec_miss = (1u << 22);
static const uint32_t mhpmevent_l1i_spec_miss_bad = (1u << 23);
static const uint32_t mhpmevent_l1i_spec_miss_good = (1u << 24);
static const uint32_t mhpmevent_l1d_ref = (1u << 25);
static const uint32_t mhpmevent_l1d_ref_r = (1u << 26);
static const uint32_t mhpmevent_l1d_ref_w = (1u << 27);
static const uint32_t mhpmevent_l1d_miss = (1u << 28);
static const uint32_t mhpmevent_l1d_miss_r = (1u << 29);
static const uint32_t mhpmevent_l1d_miss_w = (1u << 30);
static const uint32_t mhpmevent_l1d_writeback = (1u << 31);

typedef struct {
    uint64_t cycles;
    uint64_t bad_spec; // bp miss flushed cycles
    uint64_t be; // backend stall
    uint64_t be_dc; // backend stall - dcache
    uint64_t be_core; // backend stall - core
    uint64_t fe; // frontend stall
    uint64_t fe_ic; // frontend stall - dcache
    uint64_t fe_core; // frontend stall - core
    uint64_t ret; // retired inst
    uint64_t ret_simd; // retired inst - simd
    uint64_t ret_int; // retired inst - intpipe
} tda_cnt_t;

typedef struct {
    uint64_t cycles;
    uint64_t ret_branches;
    uint64_t bp_miss;
    uint64_t l1i_ref;
    uint64_t l1i_miss;
    uint64_t l1d_ref;
    uint64_t l1d_miss;
    uint64_t ret;
} hw_cnt_t;

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

void init_tda_counters();
void save_tda_counters(tda_cnt_t* pe);
void print_tda_counters(const tda_cnt_t* pe);
void print_tda_counters_json(const tda_cnt_t* pe);
void init_hw_counters();
void save_hw_counters(hw_cnt_t* p);
void print_hw_counters(const hw_cnt_t* pe);
void print_hw_counters_json(const hw_cnt_t* pe);

void trap_handler(unsigned int mcause, void* mepc, void* sp);
void timer_interrupt_handler();

#endif
