OUTPUT_ARCH( "riscv" )
ENTRY(_start)

MEMORY
{
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 128K
}

SECTIONS
{
    .text.init : { *(.text.init) }
    . = ALIGN(0x1000);
    .text : { *(.text) }
    . = ALIGN(0x1000);
    .data : { *(.data) }
    .bss : { *(.bss) }
    _end = .;
}
