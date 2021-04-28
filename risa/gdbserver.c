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
    // Create and write values to minigdbstub process call object
    mgdbProcObj mgdbObj = {0};
    mgdbObj.regs = (char*)cpu->regFile;
    mgdbObj.regsSize = sizeof(cpu->regFile);
    mgdbObj.regsCount = REGISTER_COUNT;
    mgdbObj.usrData = (void*)cpu;

    // Call into minigdbstub
    minigdbstubProcess(&mgdbObj);
}

// User-defined minigdbstub handlers

static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData) {
    return;
}

static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData) {
    return '0';
}

static void minigdbstubUsrContinue(void *usrData) {
    return;
}

static void minigdbstubUsrStep(void *usrData) {
    return;
}

static char minigdbstubUsrGetchar(void *usrData)
{
    rv32iHart *cpuHandle = (rv32iHart *)usrData;
    while (1) {
        char packet;
        size_t len = sizeof(packet);
        int err = readSocket(cpuHandle->gdbFields.connectFd, &packet, len);
        
        // TODO: Clean this up?
        return packet;
    }
}

static void minigdbstubUsrPutchar(char data, void *usrData)
{
    rv32iHart *cpuHandle = (rv32iHart *)usrData;
    writeSocket(cpuHandle->gdbFields.connectFd, (const char *)&data, sizeof(char));
}