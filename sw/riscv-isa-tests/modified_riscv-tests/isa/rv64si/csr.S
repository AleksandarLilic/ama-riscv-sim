# See LICENSE for license details.

#*****************************************************************************
# csr.S
#-----------------------------------------------------------------------------
#
# Test CSRRx and CSRRxI instructions.
#

#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64S
RVTEST_CODE_BEGIN

#ifdef __MACHINE_MODE
  #define sscratch mscratch
  #define sstatus mstatus
  #define scause mcause
  #define sepc mepc
  #define sret mret
  #define stvec_handler mtvec_handler
  #undef SSTATUS_SPP
  #define SSTATUS_SPP MSTATUS_MPP
#endif

  # For RV64, make sure UXL encodes RV64.  (UXL does not exist for RV32.)
#if __riscv_xlen == 64
  # If running in M mode, use mstatus.MPP to check existence of U mode.
  # Otherwise, if in S mode, then U mode must exist and we don't need to check.
#ifdef __MACHINE_MODE
  li t0, MSTATUS_MPP
  csrc mstatus, t0
  csrr t1, mstatus
  and t0, t0, t1
  bnez t0, 1f
#endif
  # If U mode is present, UXL should be 2 (XLEN = 64-bit)
  TEST_CASE(18, a0, SSTATUS_UXL & (SSTATUS_UXL << 1), csrr a0, sstatus; li a1, SSTATUS_UXL; and a0, a0, a1)
#ifdef __MACHINE_MODE
  j 2f
1:
  # If U mode is not present, UXL should be 0
  TEST_CASE(19, a0, 0, csrr a0, sstatus; li a1, SSTATUS_UXL; and a0, a0, a1)
2:
#endif
#endif

  TEST_CASE(20, a0,         0, csrw sscratch, zero; csrr a0, sscratch);
  TEST_CASE(21, a0,         0, csrrwi a0, sscratch, 0; csrrwi a0, sscratch, 0xF);
  TEST_CASE(22, a0,      0x1f, csrrsi x0, sscratch, 0x10; csrr a0, sscratch);

  csrwi sscratch, 3
  TEST_CASE( 2, a0,         3, csrr a0, sscratch);
  TEST_CASE( 3, a1,         3, csrrci a1, sscratch, 1);
  TEST_CASE( 4, a2,         2, csrrsi a2, sscratch, 4);
  TEST_CASE( 5, a3,         6, csrrwi a3, sscratch, 2);
  TEST_CASE( 6, a1,         2, li a0, 0xbad1dea; csrrw a1, sscratch, a0);
  TEST_CASE( 7, a1, 0xbad1dea, li a0, 0x0001dea; csrrc a1, sscratch, a0);
  TEST_CASE( 8, a1, 0xbad0000, li a0, 0x000beef; csrrs a1, sscratch, a0);
  TEST_CASE( 9, a0, 0xbadbeef, li a0, 0xbad1dea; csrrw a0, sscratch, a0);
  TEST_CASE(10, a0, 0xbad1dea, li a0, 0x0001dea; csrrc a0, sscratch, a0);
  TEST_CASE(11, a0, 0xbad0000, li a0, 0x000beef; csrrs a0, sscratch, a0);
  TEST_CASE(12, a0, 0xbadbeef, csrr a0, sscratch);

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

fsw_data: .word 1

RVTEST_DATA_END
