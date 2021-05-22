#include "risa.h"
#include "socket.h"
#include "minigdbstub.h"

void gdbserverInit(rv32iHart *cpu) {
    cpu->gdbFields.serverPort = 3333;

    if ((cpu->gdbFields.socketFd > 0) || (cpu->gdbFields.connectFd > 0)) {
        stopServer(cpu);
    }
    startServer(cpu);
    
    if (cpu->gdbFields.socketFd > 0) {
        printf("[rISA]: GDB server started.\n");
    }
    else {
        printf("[rISA]: Error - Could not start GDB server. Falling back to regular simulator execution.\n");
        cpu->opts.o_gdbEnabled = 0;
    }
    return;
}

void gdbserverCall(rv32iHart *cpu) {
    // Update regs
    u32 regs[REGISTER_COUNT+1];
    for (int i=0; i<REGISTER_COUNT; ++i) {
        regs[i] = cpu->regFile[i];
    }
    // Append PC reg
    regs[REGISTER_COUNT] = cpu->pc;

    // Create and write values to minigdbstub process call object
    mgdbProcObj mgdbObj = {0};
    mgdbObj.regs = (char*)regs;
    mgdbObj.regsSize = sizeof(regs);
    mgdbObj.regsCount = REGISTER_COUNT;
    if (cpu->cycleCounter > 0) {
        mgdbObj.opts.o_signalOnEntry = 1;
    }
    mgdbObj.opts.o_enableLogging = 0;
    mgdbObj.usrData = (void*)cpu;

    // Call into minigdbstub
    minigdbstubProcess(&mgdbObj);
}

// User-defined minigdbstub handlers
static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData) {
    rv32iHart *cpuHandle = (rv32iHart*)usrData;
    ACCESS_MEM_W(cpuHandle->virtMem, addr) = data;
    return;
}

static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData) {
    rv32iHart *cpuHandle = (rv32iHart*)usrData;
    return ACCESS_MEM_B(cpuHandle->virtMem,addr);
}

static void minigdbstubUsrContinue(void *usrData) {
    rv32iHart *cpuHandle = (rv32iHart*)usrData;
    cpuHandle->gdbFields.gdbFlags.dbgContinue = 1;
    return;
}

static void minigdbstubUsrStep(void *usrData) {
    return;
}

static char minigdbstubUsrGetchar(void *usrData)
{
    rv32iHart *cpuHandle = (rv32iHart*)usrData;
    while (1) {
        char packet;
        size_t len = sizeof(packet);
        readSocket(cpuHandle->gdbFields.connectFd, &packet, len);
        
        // TODO: Clean this up?
        return packet;
    }
}

static void minigdbstubUsrPutchar(char data, void *usrData)
{
    rv32iHart *cpuHandle = (rv32iHart *)usrData;
    writeSocket(cpuHandle->gdbFields.connectFd, (const char *)&data, sizeof(char));
}
