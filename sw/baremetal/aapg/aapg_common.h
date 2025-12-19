.macro test_entry_macro
.endm

.macro pre_program_macro
.endm
.macro post_program_macro
    csrwi 0x51e, 0x1
.endm

.macro pre_branch_macro
.endm

.macro post_branch_macro
.endm
