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
    asm volatile("csrw 0x340, " TOSTR(TESTNUM_REG));
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

int write_uart0(int file, char *ptr, int len) {
    int count = len;
    while (count-- > 0) {
        while (!UART0_TX_READY);
        UART0->tx_data = *ptr;
        ptr++;
    }
    return len;
}

// tiny printf implementation
// write character to stdout
void _putchar(char character) {  write_uart0(0, &character, 1); }
