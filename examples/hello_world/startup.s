.section .init, "ax"
.global _risa_start
.global _start

_risa_start:
    # Init the stack pointer then jump to crt0 "_start"
    la gp, __global_pointer$
    la sp, __stack_top
    add s0, sp, zero
    jal zero, _start
