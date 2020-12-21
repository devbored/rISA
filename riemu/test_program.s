START: addi x2, x0, 28
lw x14, 8(x2)
lui x3, 17
jal x7, JMP1
sw x14, 8(x2)
JMP1: jal x7, START
add x0, x0, x0
