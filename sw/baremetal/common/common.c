#include "common.h"

// void test_end(); // defined in crt0.S

void write_mismatch(uint32_t res, uint32_t ref, uint8_t idx) {
    asm volatile("add x29, x0, %0"
                 :
                 : "r"(res));
    asm volatile("add x30, x0, %0"
                 :
                 : "r"(ref));
    asm volatile("add x28, x0, %0"
                 :
                 : "r"(idx));
}

void write_csr_status() {
    asm volatile("csrw 0x51e, " TOSTR(TESTNUM_REG));
}

void pass() {
    asm volatile("li " TOSTR(TESTNUM_REG) ", 1");
    write_csr_status();
    test_end();
}

void fail() {
    asm volatile("sll " TOSTR(TESTNUM_REG) ", " TOSTR(TESTNUM_REG) ", 1");
    asm volatile("or " TOSTR(TESTNUM_REG) ", " TOSTR(TESTNUM_REG) ", 1");
    write_csr_status();
    test_end();
}

void send_byte_uart0(char byte) {
    while (!UART0_TX_READY);
    UART0->tx_data = byte;
}

int _write(int fd, char *ptr, int len) {
    int count = len;
    while (count-- > 0) {
        send_byte_uart0(*ptr);
        ptr++;
    }
    return len;
}

int __puts_uart(char *s, int len, void *buf) {
	_write( 0, s, len );
	return len;
}

int mini_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int count = mini_vpprintf(__puts_uart, NULL, format, args);
    va_end(args);
    return count;
}

uint32_t time_us() {
    uint32_t time_us;
    asm volatile("csrr %0, time" : "=r"(time_us));
    return time_us;
}

uint32_t clock_ticks() {
    uint32_t clock_ticks;
    asm volatile("csrr %0, cycle" : "=r"(clock_ticks));
    return clock_ticks;
}
