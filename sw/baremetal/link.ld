OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
    ram : ORIGIN = 0x80000000, LENGTH = 0x4000
    peripherals : ORIGIN = 0x80004000, LENGTH = 12
}

SECTIONS
{
    .text : {
        * (.start)
        * (.text)
        . = ALIGN(4);
    } > ram
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > ram
    .data : {
        __global_pointer$ = . + 0x800;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
    } > ram
    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __BSS_END__ = .;
    } > ram
}
