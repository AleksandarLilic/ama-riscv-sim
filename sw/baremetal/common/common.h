#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define STRINGIFY(x) #x
#define TOSTR(x) STRINGIFY(x)
#define TESTNUM_REG x28

void test_end(); // defined in crt0.S

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

#endif
