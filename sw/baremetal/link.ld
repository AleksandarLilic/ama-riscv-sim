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
    } > ram
    .bss : {
        *(.bss)
        *(.bss.*)
    } > ram
    .data : {
        *(.data)
        *(.data.*)
    } > ram
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    } > ram
}
