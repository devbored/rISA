#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "risa.h"

void defaultStubMmioHandler(u32 addr, u32 *virtMem, rv32iHart cpu)  { return; }
void defaultStubIntHandler(u32 *vertMem, rv32iHart cpu)             { return; }
void defaultStubEnvHandler(u32 *vertMem, rv32iHart cpu)             { return; }

int main(int argc, char** argv) {
    rv32iHart cpu = {0};
    u32 virtMem[1024] = {0};    // TODO: Add option to change this value
    const u32 memRange = sizeof(virtMem);
    if (argc == 1) {
        printf("[rISA]: No program specified.\n\tUsage: rISA <program-binary>\n\n\tExiting...\n");
        return 0;
    }
    FILE* binFile;
    OPEN_FILE(binFile, argv[1], "rb");
    if (binFile == NULL) {
        printf("[rISA]: Error. Could not open '%s'.\n\tExiting...\n", argv[1]);
        return 1;
    }
    for (int i=0; !feof(binFile) != 0; ++i) {
        if (i >= (memRange / sizeof(u32))) {
            printf("[rISA]: Error. Could not fit '%s' in emulator's virtual memory.\n\tExiting...\n", argv[1]);
            fclose(binFile);
            return 1;
        }
        fread(virtMem+i, sizeof(u32), 1, binFile);
    }
    fclose(binFile);
    // Check for user-overloaded handler functions
    cpu.pfnMmioHandler = defaultStubMmioHandler;
    cpu.pfnIntHandler  = defaultStubIntHandler;
    cpu.pfnEnvHandler  = defaultStubEnvHandler; 
    if (argc > 2) {
        printf("%s\n", argv[2]);
        LIB_HANDLE libHandle = LOAD_LIB(argv[2]);
        if (libHandle == NULL) {
            printf("[rISA]: Error. Could not load dynamic library '%s'.\n\tExiting...\n", argv[2]);
            return 1;
        }
        pfnMmioHandler mmioHandle = (pfnMmioHandler)LOAD_SYM(libHandle, "risaMmioHandler");
        if (mmioHandle != NULL) { cpu.pfnMmioHandler = mmioHandle; }
        pfnIntHandler intHandle = (pfnIntHandler)LOAD_SYM(libHandle, "risaIntHandler");
        if (mmioHandle != NULL) { cpu.pfnIntHandler = intHandle; }
        pfnEnvHandler envHandle = (pfnEnvHandler)LOAD_SYM(libHandle, "risaEnvHandler");
        if (mmioHandle != NULL) { cpu.pfnEnvHandler = envHandle; }
    }
    DEBUG_PRINT("Running simulator...\n\n");
    for (;;) {
        // Fetch
        cpu.IF = ACCESS_MEM_W(cpu.pc);
        cpu.instFields.opcode = GET_BITSET(cpu.IF, 0, 7);
        switch (OpcodeToFormat[cpu.instFields.opcode]) {
            case R: {
                // Decode
                cpu.instFields.funct3 = GET_BITSET(cpu.IF, 12, 3);
                cpu.instFields.rd     = GET_BITSET(cpu.IF, 7, 5);
                cpu.instFields.rs1    = GET_BITSET(cpu.IF, 15, 5);
                cpu.instFields.rs2    = GET_BITSET(cpu.IF, 20, 5);
                cpu.instFields.funct7 = GET_BITSET(cpu.IF, 25, 7);
                cpu.ID = (cpu.instFields.funct7 << 10) | (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                // Execute
                switch ((RtypeInstructions)cpu.ID) {
                    case SLLI: { // Shift left logical by immediate (i.e. rs2 is shamt)
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] << cpu.instFields.rs2;
                        DEBUG_PRINT("Current instruction: slli x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SRLI: { // Shift right logical by immediate (i.e. rs2 is shamt)
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] >> cpu.instFields.rs2;
                        DEBUG_PRINT("Current instruction: srli x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SRAI: { // Shift right arithmetic by immediate (i.e. rs2 is shamt)
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)((s32)cpu.regFile[cpu.instFields.rs1] >> cpu.instFields.rs2);
                        DEBUG_PRINT("Current instruction: srai x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case ADD:  { // Addition
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] + cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: add x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SUB:  { // Subtraction
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] - cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: sub x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SLL:  { // Shift left logical
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] << cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: sll x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SLT:  { // Set if less than (signed)
                        cpu.regFile[cpu.instFields.rd] = 
                            ((s32)cpu.regFile[cpu.instFields.rs1] < (s32)cpu.regFile[cpu.instFields.rs2]) ? 1 : 0;
                        DEBUG_PRINT("Current instruction: slt x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SLTU: { // Set if less than (unsigned)
                        cpu.regFile[cpu.instFields.rd] = 
                            (cpu.regFile[cpu.instFields.rs1] < cpu.regFile[cpu.instFields.rs2]) ? 1 : 0;
                        DEBUG_PRINT("Current instruction: sltu x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case XOR:  { // Bitwise xor
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] ^ cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: xor x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SRL:  { // Shift right logical
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] >> cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: srl x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SRA:  { // Shift right arithmetic
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)((s32)cpu.regFile[cpu.instFields.rs1] >> cpu.regFile[cpu.instFields.rs2]);
                        DEBUG_PRINT("Current instruction: sra x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case OR:   { // Bitwise or
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] | cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: or x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case AND:  { // Bitwise and
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] & cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: and x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                }
                break;
            }
            case I: {
                // Decode
                cpu.instFields.funct3 = GET_BITSET(cpu.IF, 12, 3);
                cpu.instFields.rd     = GET_BITSET(cpu.IF, 7, 5);
                cpu.instFields.rs1    = GET_BITSET(cpu.IF, 15, 5);
                cpu.immFields.imm11_0 = GET_BITSET(cpu.IF, 20, 12);
                cpu.immFields.succ    = GET_BITSET(cpu.IF, 20, 4);
                cpu.immFields.pred    = GET_BITSET(cpu.IF, 24, 4);
                cpu.immFields.fm      = GET_BITSET(cpu.IF, 28, 4);
                cpu.immFinal = (((s32)cpu.immFields.imm11_0 << 20) >> 20);
                cpu.ID = (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                // Execute
                switch ((ItypeInstructions)cpu.ID) {
                    case JALR:  { // Jump and link register
                        cpu.regFile[cpu.instFields.rd] = cpu.pc + 4;
                        cpu.pc = ((cpu.regFile[cpu.instFields.rs1] + cpu.immFinal) & 0xfffe) - 4;
                        DEBUG_PRINT("Current instruction: jalr x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case LB:    { // Load byte (signed)
                        u32 loadByte = (u32)ACCESS_MEM_B(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal);
                        cpu.regFile[cpu.instFields.rd] = (u32)((s32)(loadByte << 24) >> 24);
                        DEBUG_PRINT("Current instruction: lb x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case LH:    { // Load halfword (signed)
                        u32 loadHalfword = (u32)ACCESS_MEM_H(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal);
                        cpu.regFile[cpu.instFields.rd] = (u32)((s32)(loadHalfword << 16) >> 16);
                        DEBUG_PRINT("Current instruction: lh x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case LW:    { // Load word
                        cpu.regFile[cpu.instFields.rd] = 
                            ACCESS_MEM_W(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal);
                        DEBUG_PRINT("Current instruction: lw x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case LBU:   { // Load byte (unsigned)
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)ACCESS_MEM_B(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal);
                        DEBUG_PRINT("Current instruction: lbu x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case LHU:   { // Load halfword (unsigned)
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)ACCESS_MEM_H(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal);
                        DEBUG_PRINT("Current instruction: lhu x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case ADDI:  { // Add immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] + cpu.immFinal;
                        DEBUG_PRINT("Current instruction: addi x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case SLTI:  { // Set if less than immediate (signed)
                        cpu.regFile[cpu.instFields.rd] = 
                            ((s32)cpu.regFile[cpu.instFields.rs1] < cpu.immFinal) ? 1 : 0;
                        DEBUG_PRINT("Current instruction: slti x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case SLTIU: { // Set if less than immediate (unsigned)
                        cpu.regFile[cpu.instFields.rd] = 
                            (cpu.regFile[cpu.instFields.rs1] < (u32)cpu.immFinal) ? 1 : 0;
                        DEBUG_PRINT("Current instruction: sltiu x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case XORI:  { // Bitwise exclusive or immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] ^ cpu.immFinal;
                        DEBUG_PRINT("Current instruction: xori x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case ORI:   { // Bitwise or immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] | cpu.immFinal;
                        DEBUG_PRINT("Current instruction: ori x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case ANDI:  { // Bitwise and immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] & cpu.immFinal;
                        DEBUG_PRINT("Current instruction: andi x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case FENCE: { // FENCE - order device I/O and memory accesses
                        // NOP for now...
                        DEBUG_PRINT("Current instruction: fence %d, %d\n",
                            cpu.immFields.pred, cpu.immFields.succ);
                        break;
                    }
                    // Catch environment-type instructions
                    default: {
                        cpu.ID = (cpu.immFields.imm11_0 << 20) | (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                        switch ((ItypeInstructions)cpu.ID) {
                            case ECALL: { // ECALL - request a syscall
                                DEBUG_PRINT("Current instruction: ecall\n");
                                break;
                            }
                            case EBREAK: { // EBREAK - halt processor execution, transfer control to debugger
                                DEBUG_PRINT("Current instruction: ebreak\n");
                                break;
                            }
                        }
                        cpu.pfnEnvHandler(virtMem, cpu);
                    }
                }
                break;
            }
            case S: {
                // Decode
                cpu.instFields.funct3 = GET_BITSET(cpu.IF, 12, 3);
                cpu.immFields.imm4_0  = GET_BITSET(cpu.IF, 7, 5);
                cpu.instFields.rs1    = GET_BITSET(cpu.IF, 15, 5);
                cpu.instFields.rs2    = GET_BITSET(cpu.IF, 20, 5);
                cpu.immFields.imm11_5 = GET_BITSET(cpu.IF, 25, 7);
                cpu.immPartial = cpu.immFields.imm4_0 | (cpu.immFields.imm11_5 << 5);
                cpu.immFinal = (((s32)cpu.immPartial << 20) >> 20);
                cpu.ID = (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                // Execute
                switch ((StypeInstructions)cpu.ID) {
                    case SB: { // Store byte
                        ACCESS_MEM_B(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal) = 
                            (u8)cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: sb x%d, %d(x%d)\n",
                            cpu.instFields.rs2, cpu.immPartial, cpu.instFields.rs1);
                        break;
                    }
                    case SH: { // Store halfword
                        ACCESS_MEM_H(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal) = 
                            (u16)cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: sh x%d, %d(x%d)\n",
                            cpu.instFields.rs2, cpu.immPartial, cpu.instFields.rs1);
                        break;
                    }
                    case SW: { // Store word
                        ACCESS_MEM_W(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal) = 
                            cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT("Current instruction: sw x%d, %d(x%d)\n",
                            cpu.instFields.rs2, cpu.immPartial, cpu.instFields.rs1);
                        break;
                    }
                }
                cpu.pfnMmioHandler(cpu.regFile[cpu.instFields.rs1] + cpu.immFinal, virtMem, cpu);
                break;
            }
            case B: {
                // Decode
                cpu.instFields.funct3 = GET_BITSET(cpu.IF, 12, 3);
                cpu.instFields.rs1    = GET_BITSET(cpu.IF, 15, 5);
                cpu.instFields.rs2    = GET_BITSET(cpu.IF, 20, 5);
                cpu.immFields.imm11   = GET_BITSET(cpu.IF, 7, 1);
                cpu.immFields.imm4_1  = GET_BITSET(cpu.IF, 8, 4);
                cpu.immFields.imm10_5 = GET_BITSET(cpu.IF, 25, 6);
                cpu.immFields.imm12   = GET_BITSET(cpu.IF, 31, 1);
                cpu.immPartial = cpu.immFields.imm11 | (cpu.immFields.imm4_1 << 1) | 
                    (cpu.immFields.imm10_5 << 5) | (cpu.immFields.imm12 << 11);
                cpu.immFinal = (s32)(cpu.immPartial << 20) >> 19;
                cpu.ID = (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                // Execute
                switch ((BtypeInstructions)cpu.ID) {
                    case BEQ:  { // Branch if Equal
                        if ((s32)cpu.regFile[cpu.instFields.rs1] == (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT("Current instruction: beq x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BNE:  { // Branch if Not Equal
                        if ((s32)cpu.regFile[cpu.instFields.rs1] != (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT("Current instruction: bne x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BLT:  { // Branch if Less Than
                        if ((s32)cpu.regFile[cpu.instFields.rs1] < (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT("Current instruction: blt x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BGE:  { // Branch if Greater Than or Equal
                        if ((s32)cpu.regFile[cpu.instFields.rs1] >= (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT("Current instruction: bge x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BLTU: { // Branch if Less Than (unsigned)
                        if (cpu.regFile[cpu.instFields.rs1] < cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT("Current instruction: bltu x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BGEU: { // Branch if Greater Than or Equal (unsigned)
                        if (cpu.regFile[cpu.instFields.rs1] >= cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT("Current instruction: bgeu x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                }
                break;
            }
            case U: {
                // Decode
                cpu.instFields.rd      = GET_BITSET(cpu.IF, 7, 5);
                cpu.immFields.imm31_12 = GET_BITSET(cpu.IF, 12, 20);
                cpu.immFinal = cpu.immFields.imm31_12 << 12;
                // Execute
                switch ((UtypeInstructions)cpu.instFields.opcode) {
                    case LUI:   { // Load Upper Immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.immFinal;
                        DEBUG_PRINT("Current instruction: lui x%d, %d\n",
                            cpu.instFields.rd, cpu.immFinal);
                        break;
                    }
                    case AUIPC: { // Add Upper Immediate to cpu.pc
                        cpu.regFile[cpu.instFields.rd] = cpu.pc + cpu.immFinal;
                        DEBUG_PRINT("Current instruction: auipc x%d, %d\n",
                            cpu.instFields.rd, cpu.immFinal);
                        break;
                    }
                }
                break;
            }
            case J: {
                // Decode
                cpu.instFields.rd      = GET_BITSET(cpu.IF, 7, 5);
                cpu.immFields.imm19_12 = GET_BITSET(cpu.IF, 12, 8);
                cpu.immFields.imm11    = GET_BITSET(cpu.IF, 20, 1);
                cpu.immFields.imm10_1  = GET_BITSET(cpu.IF, 21, 10);
                cpu.immFields.imm20    = GET_BITSET(cpu.IF, 31, 1);
                cpu.immPartial = cpu.immFields.imm10_1 | (cpu.immFields.imm11 << 10) | 
                    (cpu.immFields.imm19_12 << 11) | (cpu.immFields.imm20 << 19);
                cpu.immFinal = (s32)(cpu.immPartial << 12) >> 11;
                // Execute
                switch ((JtypeInstructions)(cpu.instFields.opcode)) {
                    case JAL: { // Jump and link
                        cpu.regFile[cpu.instFields.rd] = cpu.pc + 4;
                        cpu.pc += cpu.immFinal - 4;
                        DEBUG_PRINT("Current instruction: jal x%d, %d\n",
                            cpu.instFields.rd, cpu.immFinal);
                        break;
                    }
                }
                break;
            }
            default: { // Invalid instruction
                DEBUG_PRINT("Error. (0x%08x) is an invalid instruction.\n", cpu.IF);
                abort();
            }
        }
        // Update cpu.pc and counter, check for interrupts, and reset register x0 back to zero
        cpu.pc += 4;
        if (cpu.pc > memRange) {
            DEBUG_PRINT("Error. Program counter is out of range.\n");
            abort();
        }
        cpu.cycleCounter++;
        if ((cpu.cycleCounter % INT_PERIOD) == 0) { 
            cpu.pfnIntHandler(virtMem, cpu); 
        }
        cpu.regFile[0] = 0;
        DEBUG_PRINT("<%d> cycle(s)\n\n", cpu.cycleCounter);
        if (cpu.cycleCounter == INT32_MAX) {
            DEBUG_PRINT("Emulator timeout reached. Exiting...\n");
            return 0;
        }
    }
    return 0;
}