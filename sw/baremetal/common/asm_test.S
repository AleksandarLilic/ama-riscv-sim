
#define CSR_TOHOST 0x51E

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

#define TEST_C0(op) \
    .option rvc;    \
    op;             \
    .option norvc

#define TEST_C1(op, v1) \
    .option rvc;        \
    op v1;              \
    .option norvc

#define TEST_C2(op, v1, v2) \
    .option rvc;            \
    op v1, v2;              \
    .option norvc

#define TEST_C3(op, v1, v2, v3) \
    .option rvc;                \
    op v1, v2, v3;              \
    .option norvc
