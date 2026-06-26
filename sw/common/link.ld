OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 128K
}

__stack_size = DEFINED(__stack_size) ? __stack_size : 16K;

SECTIONS
{
    .text : {
        * (.start)
        * (.text)
        * (.text.*)
        . = ALIGN(4);
    } > ram
    .rodata (READONLY) : {
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > ram
    .init_array : {
        __init_array_start = .;
        KEEP (*(.init_array*))
        __init_array_end = .;
    } > ram
    .fini_array : {
        __fini_array_start = .;
        KEEP (*(.fini_array*))
        __fini_array_end = .;
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
    __stack_limit = __stack_top - __stack_size;
    ASSERT(__stack_limit >= _end, "RAM overflow: heap start overlaps reserved stack")
}
