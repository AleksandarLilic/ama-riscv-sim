
#define TEST_INC      \
    addi x13, x13, 1

# result of the tested operation expected in x26
#define OP_END_C       \
    TEST_INC;          \
    bne x30, x26, fail

# result of the tested operation expected in x14 (compressed isa, short range)
#define OP_END_CS      \
    TEST_INC;          \
    bne x30, x14, fail

#define OP_END(x) \
    li x30, x;    \
    OP_END_C

#define TEST_C0(op)     \
    .option arch, +c;   \
    op;                 \
    .option arch, -c

#define TEST_C1(op, v1)     \
    .option arch, +c;       \
    op v1;                  \
    .option arch, -c

#define TEST_C2(op, v1, v2)     \
    .option arch, +c;           \
    op v1, v2;                  \
    .option arch, -c

#define TEST_C3(op, v1, v2, v3)     \
    .option arch, +c;               \
    op v1, v2, v3;                  \
    .option arch, -c

#define FAIL \
    add x28, x0, x13; \
    slli x28, x28, 1; \
    or x28, x28, 1; \
    csrw CSR_TOHOST, x28; \
    nop; \
    nop; \
    nop; \
    nop

#define PASS \
    li x28, 0x1; \
    csrw CSR_TOHOST, x28; \
    nop; \
    nop; \
    nop; \
    nop
