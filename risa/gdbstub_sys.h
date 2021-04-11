#ifndef GDBSTUB_SYS_H
#define GDBSTUB_SYS_H

#include <stdint.h>
#include "risa.h"

enum DBG_REGS {
    DBG_CPU_RISCV_REG_X0,
    DBG_CPU_RISCV_REG_X1,
    DBG_CPU_RISCV_REG_X2,
    DBG_CPU_RISCV_REG_X3,
    DBG_CPU_RISCV_REG_X4,
    DBG_CPU_RISCV_REG_X5,
    DBG_CPU_RISCV_REG_X6,
    DBG_CPU_RISCV_REG_X7,
    DBG_CPU_RISCV_REG_X8,
    DBG_CPU_RISCV_REG_X9,
    DBG_CPU_RISCV_REG_X10,
    DBG_CPU_RISCV_REG_X11,
    DBG_CPU_RISCV_REG_X12,
    DBG_CPU_RISCV_REG_X13,
    DBG_CPU_RISCV_REG_X14,
    DBG_CPU_RISCV_REG_X15,
    DBG_CPU_RISCV_REG_X16,
    DBG_CPU_RISCV_REG_X17,
    DBG_CPU_RISCV_REG_X18,
    DBG_CPU_RISCV_REG_X19,
    DBG_CPU_RISCV_REG_X20,
    DBG_CPU_RISCV_REG_X21,
    DBG_CPU_RISCV_REG_X22,
    DBG_CPU_RISCV_REG_X23,
    DBG_CPU_RISCV_REG_X24,
    DBG_CPU_RISCV_REG_X25,
    DBG_CPU_RISCV_REG_X26,
    DBG_CPU_RISCV_REG_X27,
    DBG_CPU_RISCV_REG_X28,
    DBG_CPU_RISCV_REG_X29,
    DBG_CPU_RISCV_REG_X30,
    DBG_CPU_RISCV_REG_X31,
    DBG_CPU_RISCV_REG_PC,
    DBG_CPU_RISCV_NUM_REGS,

    // gdbstub quirk - the function "dbg_main(...)" expects and uses DBG_CPU_I386_NUM_REGISTERS
    // which rISA does not define, so set DBG_CPU_I386_NUM_REGISTERS as a crude alias to DBG_CPU_RISCV_NUM_REGS
    DBG_CPU_I386_NUM_REGISTERS = DBG_CPU_RISCV_NUM_REGS
};

typedef uint32_t address;
typedef uint32_t reg;

typedef struct dbg_state {
	int signum;
	reg registers[DBG_CPU_RISCV_NUM_REGS-1];
    rv32iHart *cpuHandle;
} dbg_state;

typedef struct dbg_msg {
    enum {
        DBG_CONTINUE,
        DBG_SET_BREAK,
        DBG_REMOVE_BREAK,
        DBG_STEP,
        DBG_ACK,
        DBG_EXIT,
        DBG_HIT,
        DBG_BREAK
    } type;
    address addr;
} dbg_msg;

void dbg_sys_start(rv32iHart *cpu);
void dbg_sys_process(void);

#endif // GDBSTUB_SYS_H