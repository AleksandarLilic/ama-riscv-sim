OUTPUT_ARCH( "riscv" )
ENTRY(_start)

MEMORY
{
    ram (rwx) : ORIGIN = 0x40000, LENGTH = 64K
    peripherals (rw) : ORIGIN = ORIGIN(ram) + LENGTH(ram), LENGTH = 12
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
