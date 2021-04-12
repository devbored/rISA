#include "risa.h"
#include "socket.h"
#include "gdbstub.h"
#include "gdbstub_sys.h"

static dbg_state g_dbgHandle = {0};

static void updateRegs(dbg_state *dbgState) {
    for (int i=0; i<DBG_CPU_RISCV_NUM_REGS-1; ++i) {
        dbgState->registers[i] = dbgState->cpuHandle->regFile[i];
    }
    dbgState->registers[DBG_CPU_RISCV_REG_PC] = dbgState->cpuHandle->pc;
}

void dbg_sys_process(void) {
    updateRegs(&g_dbgHandle);

    // Call into gdbstub
    dbg_main(&g_dbgHandle);
}

void dbg_sys_start(rv32iHart *cpu) {
    cpu->gdbFields.serverPort = 3333;

    if ((cpu->gdbFields.socketFd > 0) || (cpu->gdbFields.connectFd > 0)) {
        stopServer(cpu);
    }
    startServer(cpu);
    
    if (cpu->gdbFields.socketFd > 0) {
        printf("[rISA]: GDB server started.\n");
        g_dbgHandle.cpuHandle = cpu;
    }
    else {
        printf("[rISA]: Error - Could not start GDB server. Falling back to regular simulator execution.\n");
        cpu->opts.o_gdbEnabled = 0;
        return;
    }
}

int dbg_sys_continue(void)
{
    return 0;
}

int dbg_sys_step(void)
{
    return 0;
}

int dbg_sys_mem_readb(address addr, char *val)
{
    return 0;
}

int dbg_sys_mem_writeb(address addr, char val)
{
    return 0;
}

int dbg_sys_getc(void)
{
    while (1) {
        char packet;
        size_t len = sizeof(packet);
        int err = readSocket(g_dbgHandle.cpuHandle->gdbFields.connectFd, &packet, len);
        
        // TODO: Clean this up?
        return packet;
    }
}

int dbg_sys_putchar(int ch)
{
    writeSocket(g_dbgHandle.cpuHandle->gdbFields.connectFd, (const char *)&ch, sizeof(char));
}