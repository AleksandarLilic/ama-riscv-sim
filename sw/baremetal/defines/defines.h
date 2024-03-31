//-----------------------------------------------------------------------
// Pass/Fail Macro
//-----------------------------------------------------------------------

#define RVTEST_PASS                                                     \
        li TESTNUM, 1;                                                  \
        TEST_END

#define TESTNUM x28
#define RVTEST_FAIL                                                     \
        sll TESTNUM, TESTNUM, 1;                                        \
        or TESTNUM, TESTNUM, 1;                                         \
        TEST_END

#define TEST_END                                                        \
        csrw 0x340, TESTNUM;                                            \
        nop;                                                            \
        nop;                                                            \
        nop;                                                            \
        nop;                                                            \
        ecall
