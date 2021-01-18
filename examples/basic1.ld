MEMORY {
    my_text (rwx) : ORIGIN = 0x0, LENGTH = 256
}

SECTIONS {
    .text : {
        *(.text .text*)
        *(.rodata .rodata*)
    } > my_text
}