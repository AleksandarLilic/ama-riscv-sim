#include "common.h"

void send_byte_uart0(char byte) {
    while (!UART0_TX_READY);
    UART0->tx_data = byte;
}

int _write(int fd, char* ptr, int len) {
    (void)fd;
    int count = len;
    while (count-- > 0) {
        send_byte_uart0(*ptr);
        ptr++;
    }
    return len;
}

int __puts_uart(char *s, int len, void* buf) {
    (void)buf;
    _write(0, s, len);
    return len;
}

int mini_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int count = mini_vpprintf(__puts_uart, NULL, format, args);
    va_end(args);
    return count;
}

uint64_t read_csr_wide(uint32_t csr_lo, uint32_t csr_hi) {
    uint32_t lo, hi, hi2;
    sliced64_t val;
    while (1) {
        hi = read_csr(csr_hi);
        lo = read_csr(csr_lo);
        hi2 = read_csr(csr_hi);
        if (hi == hi2) {
            val.u32[0] = lo;
            val.u32[1] = hi;
            break;
        }
    }
    return val.u64;
}

uint64_t write_csr_wide(uint32_t csr_lo, uint32_t csr_hi, uint64_t value) {
    sliced64_t val = { .u64 = value };
    write_csr(csr_hi, val.u32[1]);
    write_csr(csr_lo, val.u32[0]);
    return value;
}

void set_cpu_time(uint64_t value) {
    write_csr_wide(CSR_TIME, CSR_TIMEH, value);
}

uint64_t get_cpu_time() {
    return read_csr_wide(CSR_TIME, CSR_TIMEH);
}

void set_cpu_cycles(uint64_t value) {
    write_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, value);
}

uint64_t get_cpu_cycles() {
    return read_csr_wide(CSR_MCYCLE, CSR_MCYCLEH);
}

void set_cpu_instret(uint64_t value) {
    write_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, value);
}

uint64_t get_cpu_instret() {
    return read_csr_wide(CSR_MINSTRET, CSR_MINSTRETH);
}

void set_up_perf_counters() {
    // reset existing event counters
    write_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, 0u);
    write_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, 0u);
    // set up events
    write_csr(CSR_MHPMEVENT3, mhpmevent_bad_spec);
    write_csr(CSR_MHPMEVENT4, mhpmevent_fe);
    write_csr(CSR_MHPMEVENT5, mhpmevent_be);
    write_csr(CSR_MHPMEVENT6, mhpmevent_fe_ic);
    write_csr(CSR_MHPMEVENT7, mhpmevent_be_dc);
    write_csr(CSR_MHPMEVENT8, mhpmevent_ret_simd);
}

void save_perf_counters(perf_event_cnt_t* p) {
    p->bad_spec = read_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H);
    p->fe = read_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H);
    p->be = read_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H);
    p->fe_ic = read_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H);
    p->be_dc = read_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H);
    p->ret_simd = read_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H);
    p->cycles = read_csr_wide(CSR_MCYCLE, CSR_MCYCLEH);
    // figure out the rest
    p->be_core = (p->be - p->be_dc);
    p->fe_core = (p->fe - p->fe_ic);
    p->ret = (p->cycles - (p->bad_spec + p->fe + p->be));
}

void print_perf_counters(const perf_event_cnt_t* p) {
    printf(
        "TDA:\n"
        "    L1: "
        "Bad Spec: %u, FE: %u, BE: %u, Retired: %u\n"
        "    L2: "
        "FE ICache: %u, FE Core: %u, "
        "BE DCache: %u, BE Core: %u, SIMD retired: %u\n"
        "    Total Cycles: %u\n",
        (uint32_t)p->bad_spec, (uint32_t)p->fe,(uint32_t)p->be,(uint32_t)p->ret,
        (uint32_t)p->fe_ic, (uint32_t)p->fe_core,
        (uint32_t)p->be_dc, (uint32_t)p->be_core, (uint32_t)p->ret_simd,
        (uint32_t)p->cycles
    );
}

void __attribute__((weak))
trap_handler(unsigned int mcause, void* mepc, void* sp) {
    (void)mepc;
    (void)sp;

    if (mcause < 0x80000000) { // can't handle exceptions, just exit
        write_mismatch(0, 0, 1000 + mcause);
        fail();
    } else { // interrupts
        if (mcause == MCAUSE_MACHINE_TIMER_INT) {
            timer_interrupt_handler();
            return;
        }
        // other unsupported atm
        write_mismatch(0, 0, 2000 + mcause);
        fail();
    }
    return;
}

void __attribute__((weak))
timer_interrupt_handler() {
    CLINT->mtimecmp += 10000; // 10ms slices
}
