OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 0x40000
    peripherals (rw) : ORIGIN = 0x80040000, LENGTH = 12
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
        __DATA_BEGIN__ = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
    } > ram
    .sdata : {
        __SDATA_BEGIN__ = .;
        *(.srodata)
        *(.srodata.*)
        *(.sdata)
        *(.sdata.*)
        . = ALIGN(4);
    } > ram
    __bss_start = .;
    .sbss : {
        *(.sbss)
        *(.sbss.*)
        *(.scommon)
        . = ALIGN(4);
    } > ram
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
    } > ram
    __BSS_END__ = .;
    __global_pointer$ = MIN(__SDATA_BEGIN__ + 0x800,
                    MAX(__DATA_BEGIN__ + 0x800, __BSS_END__ - 0x800));

    _end = .;
    __stack_top = ORIGIN(ram) + LENGTH(ram);
    __uart0_base = ORIGIN(peripherals);
}
