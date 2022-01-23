.section .text.startup
.global _start
.global _risa_start

_risa_start:
    # Init the stack pointer then jump to crt0 "_start"
    la sp, _estack
    add s0, sp, zero
    j _start
