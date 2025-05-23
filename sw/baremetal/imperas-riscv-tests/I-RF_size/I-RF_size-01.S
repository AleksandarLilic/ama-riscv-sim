# RISC-V Compliance Test I-RF_size-01
#
# Copyright (c) 2017, Codasip Ltd.
# Copyright (c) 2021, Imperas Software Ltd. Additions
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in the
#        documentation and/or other materials provided with the distribution.
#      * Neither the name of the Codasip Ltd., Imperas Software Ltd. nor the
#        names of its contributors may be used to endorse or promote products
#        derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Codasip Ltd., Imperas Software Ltd.
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Specification: RV32I Base Integer Instruction Set, Version 2.0
# Description: Testing size of register file.


#include "../common/asm_test.S"
#include "../common/csr.h"

#include "model_test.h"
#include "arch_test.h"
RVTEST_ISA("RV32I")

.section .text
.global main
main:

#ifdef TEST_CASE_1

RVTEST_CASE(0,"//check ISA:=regex(.*32.*);check ISA:=regex(.*I.*C.*);def TEST_CASE_1=True;",ecall)

    RVMODEL_IO_ASSERT_GPR_EQ(x31, x0, 0x00000000)
    RVMODEL_IO_WRITE_STR(x31, "Test Begin\n")

    # ---------------------------------------------------------------------------------------------
    RVMODEL_IO_WRITE_STR(x31, "Test part A1 - test registers x0 - x15\n")

    # Address for test results
    RVTEST_SIGBASE( x16, test_A1_res)

inst_0:

    # Test
    li      x0, 0x42726e6f
    li      x1, 0x56333249
    li      x2, 0x2d562052
    li      x3, 0x52697363
    li      x4, 0x736b7920
    li      x5, 0x73746572
    li      x6, 0x6e204e6f
    li      x7, 0x4d696c61
    li      x8, 0x6f646173
    li      x9, 0x6b794063
    li      x10, 0x74657273
    li      x11, 0x286e6f73
    li      x12, 0x656b2048
    li      x13, 0x20526164
    li      x14, 0x6f6d292c
    li      x15, 0x69702e63

    # Store results
    sw      x0, 0(x16)
    sw      x1, 4(x16)
    sw      x2, 8(x16)
    sw      x3, 12(x16)
    sw      x4, 16(x16)
    sw      x5, 20(x16)
    sw      x6, 24(x16)
    sw      x7, 28(x16)
    sw      x8, 32(x16)
    sw      x9, 36(x16)
    sw      x10, 40(x16)
    sw      x11, 44(x16)
    sw      x12, 48(x16)
    sw      x13, 52(x16)
    sw      x14, 56(x16)
    sw      x15, 60(x16)

    //
    // Assert
    //
    RVMODEL_IO_CHECK()
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x0, 0x00000000)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x1, 0x56333249)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x2, 0x2D562052)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x3, 0x52697363)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x4, 0x736B7920)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x5, 0x73746572)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x6, 0x6E204E6F)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x7, 0x4D696C61)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x8, 0x6F646173)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x9, 0x6B794063)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x10, 0x74657273)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x11, 0x286E6F73)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x12, 0x656B2048)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x13, 0x20526164)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x14, 0x6F6D292C)
    RVMODEL_IO_ASSERT_GPR_EQ(x16, x15, 0x69702E63)

    RVMODEL_IO_WRITE_STR(x31, "Test part A1  - Complete\n")

    # ---------------------------------------------------------------------------------------------
    RVMODEL_IO_WRITE_STR(x31, "Test part A2 - test registers x16 - x30\n")

    # Address for test results
    RVTEST_SIGBASE( x4, test_A2_res)

inst_1:

    # Test
    li      x16, 0x636f6461
    li      x17, 0x6a656b40
    li      x18, 0x20286861
    li      x19, 0x616a656b
    li      x20, 0x61766520
    li      x21, 0x2e204c65
    li      x22, 0x636f6d29
    li      x23, 0x7369702e
    li      x24, 0x6620796f
    li      x25, 0x67652069
    li      x26, 0x65737361
    li      x27, 0x7573206d
    li      x28, 0x3a290d0a
    li      x29, 0x68697320
    li      x30, 0x61642074
    li      x31, 0x75207265

    # Store results
    sw      x16, 0(x4)
    sw      x17, 4(x4)
    sw      x18, 8(x4)
    sw      x19, 12(x4)
    sw      x20, 16(x4)
    sw      x21, 20(x4)
    sw      x22, 24(x4)
    sw      x23, 28(x4)
    sw      x24, 32(x4)
    sw      x25, 36(x4)
    sw      x26, 40(x4)
    sw      x27, 44(x4)
    sw      x28, 48(x4)
    sw      x29, 52(x4)
    sw      x30, 56(x4)
    sw      x31, 60(x4)

    RVMODEL_IO_ASSERT_GPR_EQ(x4, x16, 0x636F6461)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x17, 0x6A656B40)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x18, 0x20286861)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x19, 0x616A656B)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x20, 0x61766520)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x21, 0x2E204C65)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x22, 0x636F6D29)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x23, 0x7369702E)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x24, 0x6620796F)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x25, 0x67652069)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x26, 0x65737361)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x27, 0x7573206D)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x28, 0x3A290D0A)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x29, 0x68697320)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x30, 0x61642074)
    RVMODEL_IO_ASSERT_GPR_EQ(x4, x31, 0x75207265)

    RVMODEL_IO_WRITE_STR(x31, "Test part A2  - Complete\n")

    # ---------------------------------------------------------------------------------------------
    RVMODEL_IO_WRITE_STR(x31, "Test part A3 - test registers x0 - x3 again (were not overwritten)\n")

    # Address for test results
    RVTEST_SIGBASE( x4, test_A3_res)

inst_2:

    # Test
    sw      x0, 0(x4)
    sw      x1, 4(x4)
    sw      x2, 8(x4)
    sw      x3, 12(x4)

    RVMODEL_IO_WRITE_STR(x31, "Test part A3  - Complete\n")

    RVMODEL_IO_WRITE_STR(x31, "Test End\n")

#endif

 # ---------------------------------------------------------------------------------------------

    # HALT
    RVMODEL_HALT

RVTEST_CODE_END


# Input data section.
RVTEST_DATA_BEGIN
    .align 4
RVTEST_DATA_END

# Output data section.
RVMODEL_DATA_BEGIN
    .align 4

test_A1_res:
    .fill 16, 4, -1
test_A2_res:
    .fill 16, 4, -1
test_A3_res:
    .fill 4, 4, -1

RVMODEL_DATA_END

