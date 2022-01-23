/* Default linker script for rISA */

/* Can modify these values */
/* Make sure to config rISA's virtual mem size to accommodate whatever size chosen */
_RomSize    = 0x2000; /* Program mem section - 4KB */
_RamSize    = 0x2000; /* RAM section - 4KB */
_StackSize  = 0x1000; /* Reserve half of RAM for the stack - 2KB */

/* -------------------------------------------------------------------------*/
/* Should not need to mess with any of the below items in normal usecases   */
/* unless you want to experiment with the memory and sections layouts       */
/* -------------------------------------------------------------------------*/

_RomStart   = 0x0;
_RamStart   = _RomSize;

MEMORY {
    rom (rwx)   : ORIGIN = _RomStart, LENGTH = _RomSize - 1
    ram (rwx)   : ORIGIN = _RamStart, LENGTH = _RamSize
}

ENTRY(_risa_start)

SECTIONS {
    .text : {
        *(.text.startup .text.startup.*)
        *(.text .text*)
        *(.rodata .rodata*)
        _etext = .;
    } > rom

    .bss (NOLOAD) :
    {
        _sbss = .;
        *(.bss .bss*)
        *(COMMON)
        _ebss = .;
    } > ram

    .data :
    {
        _sdata = .;
        __global_pointer$ = . + 0x800;
        *(.data .data*)
        _edata = .;
    } > ram AT > rom

    .stack (NOLOAD) :
    {
        . = ALIGN(4);
        _sstack = .;
        . = . + _StackSize;
        . = ALIGN(4);
        _estack = .;
    } > ram

    _end = .;
}
