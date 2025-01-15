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

uint64_t get_cpu_time() {
    uint32_t time, timeh, timeh2;
    sliced64_t time_us;
    while (1) {
        timeh = read_csr(CSR_TIMEH);
        time = read_csr(CSR_TIME);
        timeh2 = read_csr(CSR_TIMEH);
        if (timeh == timeh2) {
            time_us.u32[0] = time;
            time_us.u32[1] = timeh;
            break;
        }
    }
    return time_us.u64;
}

uint64_t get_cpu_cycles() {
    uint32_t cycles, cyclesh, cyclesh2;
    sliced64_t cycles_out;
    while (1) {
        cyclesh = read_csr(CSR_MCYCLEH);
        cycles = read_csr(CSR_MCYCLE);
        cyclesh2 = read_csr(CSR_MCYCLEH);
        if (cyclesh == cyclesh2) {
            cycles_out.u32[0] = cycles;
            cycles_out.u32[1] = cyclesh;
            break;
        }
    }
    return cycles_out.u64;
}

uint64_t get_cpu_instret() {
    uint32_t instret, instreth, instreth2;
    sliced64_t instret_out;
    while (1) {
        instreth = read_csr(CSR_MINSTRETH);
        instret = read_csr(CSR_MINSTRET);
        instreth2 = read_csr(CSR_MINSTRETH);
        if (instreth == instreth2) {
            instret_out.u32[0] = instret;
            instret_out.u32[1] = instreth;
            break;
        }
    }
    return instret_out.u64;
}

void set_cpu_cycles(uint64_t value) {
    sliced64_t cycles = { .u64 = value };
    write_csr(CSR_MCYCLE, cycles.u32[0]);
    write_csr(CSR_MCYCLEH, cycles.u32[1]);
}

void set_cpu_instret(uint64_t value) {
    sliced64_t instret = { .u64 = value };
    write_csr(CSR_MINSTRET, instret.u32[0]);
    write_csr(CSR_MINSTRETH, instret.u32[1]);
}

void trap_handler(unsigned int mcause, void* mepc, void* sp) {
    (void)mepc;
    (void)sp;
    // can't handle exceptions, just exit
    if (mcause < 0x80000000) {
        write_mismatch(0, 0, 1000 + mcause);
        fail();
    }
    // TODO: handle interrupts when supported, fail for now with different code
    write_mismatch(0, 0, 2000 + mcause);
    fail();
    return;
}
