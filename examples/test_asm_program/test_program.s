.section .text
.align 2
.globl _start
_start: addi x2, x0, 28
lw x14, 8(x2)
lui x6, %hi(STR1)
addi x6, x6, %lo(STR1)
lw x8, 0(x6)
jal x7, JMP1
sw x14, 8(x2)
JMP1: jal x7, _start
add x0, x0, x0

.section .rodata
.align 2
STR1: .string "Hello!"
