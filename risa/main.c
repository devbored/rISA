#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "risa.h"
#include "gdbstub_sys.h"

static volatile int g_sigIntDet = 0;
static SIGINT_RET_TYPE sigintHandler(SIGINT_PARAM sig) { 
    g_sigIntDet = 1;
    SIGINT_RET;
}

int main(int argc, char** argv) {
    rv32iHart cpu = {0};
    setupSimulator(argc, argv, &cpu);
    SIGINT_REGISTER(&cpu, sigintHandler);
    printf("[rISA]: Running simulator...\n\n");
    cpu.startTime = clock();
    for (;;) {
        if (cpu.opts.o_gdbEnabled) {
            dbg_sys_process();
        }

        // Fetch
        cpu.IF = ACCESS_MEM_W(cpu.virtMem, cpu.pc);
        cpu.instFields.opcode = GET_OPCODE(cpu.IF);
        switch (OpcodeToFormat[cpu.instFields.opcode]) {
            case R: {
                // Decode
                cpu.instFields.rd     = GET_RD(cpu.IF);
                cpu.instFields.rs1    = GET_RS1(cpu.IF);
                cpu.instFields.rs2    = GET_RS2(cpu.IF);
                cpu.instFields.funct3 = GET_FUNCT3(cpu.IF);
                cpu.instFields.funct7 = GET_FUNCT7(cpu.IF);
                cpu.ID = (cpu.instFields.funct7 << 10) | (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                // Execute
                switch ((RtypeInstructions)cpu.ID) {
                    case SLLI: { // Shift left logical by immediate (i.e. rs2 is shamt)
                        TRACE_R((&cpu), "slli");
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] << cpu.instFields.rs2;
                        break;
                    }
                    case SRLI: { // Shift right logical by immediate (i.e. rs2 is shamt)
                        TRACE_R((&cpu), "srli");
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] >> cpu.instFields.rs2;
                        break;
                    }
                    case SRAI: { // Shift right arithmetic by immediate (i.e. rs2 is shamt)
                        TRACE_R((&cpu), "srai");
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)((s32)cpu.regFile[cpu.instFields.rs1] >> cpu.instFields.rs2);
                        break;
                    }
                    case ADD:  { // Addition
                        TRACE_R((&cpu), "add");
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] + cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                    case SUB:  { // Subtraction
                        TRACE_R((&cpu), "sub");
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] - cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                    case SLL:  { // Shift left logical
                        TRACE_R((&cpu), "sll");
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] << cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                    case SLT:  { // Set if less than (signed)
                        TRACE_R((&cpu), "slt");
                        cpu.regFile[cpu.instFields.rd] = 
                            ((s32)cpu.regFile[cpu.instFields.rs1] < (s32)cpu.regFile[cpu.instFields.rs2]) ? 1 : 0;
                        break;
                    }
                    case SLTU: { // Set if less than (unsigned)
                        TRACE_R((&cpu), "sltu");
                        cpu.regFile[cpu.instFields.rd] = 
                            (cpu.regFile[cpu.instFields.rs1] < cpu.regFile[cpu.instFields.rs2]) ? 1 : 0;
                        break;
                    }
                    case XOR:  { // Bitwise xor
                        TRACE_R((&cpu), "xor");
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] ^ cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                    case SRL:  { // Shift right logical
                        TRACE_R((&cpu), "srl");
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] >> cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                    case SRA:  { // Shift right arithmetic
                        TRACE_R((&cpu), "sra");
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)((s32)cpu.regFile[cpu.instFields.rs1] >> cpu.regFile[cpu.instFields.rs2]);
                        break;
                    }
                    case OR:   { // Bitwise or
                        TRACE_R((&cpu), "or");
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] | cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                    case AND:  { // Bitwise and
                        TRACE_R((&cpu), "and");
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] & cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                }
                break;
            }
            case I: {
                // Decode
                cpu.instFields.rd     = GET_RD(cpu.IF);
                cpu.instFields.rs1    = GET_RS1(cpu.IF);
                cpu.instFields.funct3 = GET_FUNCT3(cpu.IF);
                cpu.immFields.imm11_0 = GET_IMM_11_0(cpu.IF);
                cpu.immFields.succ    = GET_SUCC(cpu.IF);
                cpu.immFields.pred    = GET_PRED(cpu.IF);
                cpu.immFields.fm      = GET_FM(cpu.IF);
                cpu.immFinal = (((s32)cpu.immFields.imm11_0 << 20) >> 20);
                cpu.ID = (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                cpu.targetAddress = cpu.regFile[cpu.instFields.rs1] + cpu.immFinal;
                // Execute
                switch ((ItypeInstructions)cpu.ID) {
                    case JALR:  { // Jump and link register
                        TRACE_I((&cpu), "jalr");
                        cpu.regFile[cpu.instFields.rd] = cpu.pc + 4;
                        cpu.pc = ((cpu.targetAddress) & 0xfffe) - 4;
                        break;
                    }
                    case LB:    { // Load byte (signed)
                        TRACE_L((&cpu), "lb");
                        u32 loadByte = (u32)ACCESS_MEM_B(cpu.virtMem, cpu.targetAddress);
                        cpu.regFile[cpu.instFields.rd] = (u32)((s32)(loadByte << 24) >> 24);
                        break;
                    }
                    case LH:    { // Load halfword (signed)
                        TRACE_L((&cpu), "lh");
                        u32 loadHalfword = (u32)ACCESS_MEM_H(cpu.virtMem, cpu.targetAddress);
                        cpu.regFile[cpu.instFields.rd] = (u32)((s32)(loadHalfword << 16) >> 16);
                        break;
                    }
                    case LW:    { // Load word
                        TRACE_L((&cpu), "lw");
                        cpu.regFile[cpu.instFields.rd] = 
                            ACCESS_MEM_W(cpu.virtMem, cpu.targetAddress);
                        break;
                    }
                    case LBU:   { // Load byte (unsigned)
                        TRACE_L((&cpu), "lbu");
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)ACCESS_MEM_B(cpu.virtMem, cpu.targetAddress);
                        break;
                    }
                    case LHU:   { // Load halfword (unsigned)
                        TRACE_L((&cpu), "lhu");
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)ACCESS_MEM_H(cpu.virtMem, cpu.targetAddress);
                        break;
                    }
                    case ADDI:  { // Add immediate
                        TRACE_I((&cpu), "addi");
                        cpu.regFile[cpu.instFields.rd] = cpu.targetAddress;
                        break;
                    }
                    case SLTI:  { // Set if less than immediate (signed)
                        TRACE_I((&cpu), "slti");
                        cpu.regFile[cpu.instFields.rd] = 
                            ((s32)cpu.regFile[cpu.instFields.rs1] < cpu.immFinal) ? 1 : 0;
                        break;
                    }
                    case SLTIU: { // Set if less than immediate (unsigned)
                        TRACE_I((&cpu), "sltiu");
                        cpu.regFile[cpu.instFields.rd] = 
                            (cpu.regFile[cpu.instFields.rs1] < (u32)cpu.immFinal) ? 1 : 0;
                        break;
                    }
                    case XORI:  { // Bitwise exclusive or immediate
                        TRACE_I((&cpu), "xori");
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] ^ cpu.immFinal;
                        break;
                    }
                    case ORI:   { // Bitwise or immediate
                        TRACE_I((&cpu), "ori");
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] | cpu.immFinal;
                        break;
                    }
                    case ANDI:  { // Bitwise and immediate
                        TRACE_I((&cpu), "andi");
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] & cpu.immFinal;
                        break;
                    }
                    case FENCE: { // FENCE - order device I/O and memory accesses
                        TRACE_FEN((&cpu), "fence");
                        cpu.pfnEnvHandler(&cpu);
                        break;
                    }
                    // Catch environment-type instructions
                    default: {
                        cpu.ID = (cpu.immFields.imm11_0 << 20) | (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                        switch ((ItypeInstructions)cpu.ID) {
                            case ECALL:  { // ECALL - request a syscall
                                TRACE_E((&cpu), "ecall");
                                cpu.pfnEnvHandler(&cpu);
                                break;
                            }
                            case EBREAK: { // EBREAK - halt processor execution, transfer control to debugger
                                TRACE_E((&cpu), "ebreak");
                                cpu.pfnEnvHandler(&cpu);
                                break;
                            }
                            default: { // Invalid instruction
                                printf("[rISA]: Error - (0x%08x) is an invalid instruction.\n", cpu.IF);
                                cpu.endTime = clock();
                                cleanupSimulator(&cpu, EILSEQ);
                            }
                        }
                    }
                }
                break;
            }
            case S: {
                // Decode
                cpu.instFields.funct3 = GET_FUNCT3(cpu.IF);
                cpu.immFields.imm4_0  = GET_IMM_4_0(cpu.IF);
                cpu.instFields.rs1    = GET_RS1(cpu.IF);
                cpu.instFields.rs2    = GET_RS2(cpu.IF);
                cpu.immFields.imm11_5 = GET_IMM_11_5(cpu.IF);
                cpu.immPartial = cpu.immFields.imm4_0 | (cpu.immFields.imm11_5 << 5);
                cpu.immFinal = (((s32)cpu.immPartial << 20) >> 20);
                cpu.ID = (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                cpu.targetAddress = cpu.regFile[cpu.instFields.rs1] + cpu.immFinal;
                // Execute
                switch ((StypeInstructions)cpu.ID) {
                    case SB: { // Store byte
                        TRACE_S((&cpu), "sb");
                        ACCESS_MEM_B(cpu.virtMem, cpu.targetAddress) =
                            (u8)cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                    case SH: { // Store halfword
                        TRACE_S((&cpu), "sh");
                        ACCESS_MEM_H(cpu.virtMem, cpu.targetAddress) =
                            (u16)cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                    case SW: { // Store word
                        TRACE_S((&cpu), "sw");
                        ACCESS_MEM_W(cpu.virtMem, cpu.targetAddress) =
                            cpu.regFile[cpu.instFields.rs2];
                        break;
                    }
                }
                cpu.pfnMmioHandler(&cpu);
                break;
            }
            case B: {
                // Decode
                cpu.instFields.rs1    = GET_RS1(cpu.IF);
                cpu.instFields.rs2    = GET_RS2(cpu.IF);
                cpu.instFields.funct3 = GET_FUNCT3(cpu.IF);
                cpu.immFields.imm11   = GET_IMM_11_B(cpu.IF);
                cpu.immFields.imm4_1  = GET_IMM_4_1(cpu.IF);
                cpu.immFields.imm10_5 = GET_IMM_10_5(cpu.IF);
                cpu.immFields.imm12   = GET_IMM_12(cpu.IF);
                cpu.immPartial = cpu.immFields.imm11 | (cpu.immFields.imm4_1 << 1) | 
                    (cpu.immFields.imm10_5 << 5) | (cpu.immFields.imm12 << 11);
                cpu.targetAddress = (s32)(cpu.immPartial << 20) >> 19;
                cpu.ID = (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                // Execute
                switch ((BtypeInstructions)cpu.ID) {
                    case BEQ:  { // Branch if Equal
                        TRACE_B((&cpu), "beq");
                        if ((s32)cpu.regFile[cpu.instFields.rs1] == (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.targetAddress - 4;
                        }
                        break;
                    }
                    case BNE:  { // Branch if Not Equal
                        TRACE_B((&cpu), "bne");
                        if ((s32)cpu.regFile[cpu.instFields.rs1] != (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.targetAddress - 4;
                        }
                        break;
                    }
                    case BLT:  { // Branch if Less Than
                        TRACE_B((&cpu), "blt");
                        if ((s32)cpu.regFile[cpu.instFields.rs1] < (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.targetAddress - 4;
                        }
                        break;
                    }
                    case BGE:  { // Branch if Greater Than or Equal
                        TRACE_B((&cpu), "bge");
                        if ((s32)cpu.regFile[cpu.instFields.rs1] >= (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.targetAddress - 4;
                        }
                        break;
                    }
                    case BLTU: { // Branch if Less Than (unsigned)
                        TRACE_B((&cpu), "bltu");
                        if (cpu.regFile[cpu.instFields.rs1] < cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.targetAddress - 4;
                        }
                        break;
                    }
                    case BGEU: { // Branch if Greater Than or Equal (unsigned)
                        TRACE_B((&cpu), "bgeu");
                        if (cpu.regFile[cpu.instFields.rs1] >= cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.targetAddress - 4;
                        }
                        break;
                    }
                }
                break;
            }
            case U: {
                // Decode
                cpu.instFields.rd      = GET_RD(cpu.IF);
                cpu.immFields.imm31_12 = GET_IMM_31_12(cpu.IF);
                cpu.immFinal = cpu.immFields.imm31_12 << 12;
                // Execute
                switch ((UtypeInstructions)cpu.instFields.opcode) {
                    case LUI:   { // Load Upper Immediate
                        TRACE_U((&cpu), "lui");
                        cpu.regFile[cpu.instFields.rd] = cpu.immFinal;
                        break;
                    }
                    case AUIPC: { // Add Upper Immediate to cpu.pc
                        TRACE_U((&cpu), "auipc");
                        cpu.regFile[cpu.instFields.rd] = cpu.pc + cpu.immFinal;
                        break;
                    }
                }
                break;
            }
            case J: { // Jump and link
                // Decode
                cpu.instFields.rd      = GET_RD(cpu.IF);
                cpu.immFields.imm19_12 = GET_IMM_19_12(cpu.IF);
                cpu.immFields.imm11    = GET_IMM_11_J(cpu.IF);
                cpu.immFields.imm10_1  = GET_IMM_10_1(cpu.IF);
                cpu.immFields.imm20    = GET_IMM_20(cpu.IF);
                cpu.immPartial = cpu.immFields.imm10_1 | (cpu.immFields.imm11 << 10) | 
                    (cpu.immFields.imm19_12 << 11) | (cpu.immFields.imm20 << 19);
                cpu.targetAddress = (s32)(cpu.immPartial << 12) >> 11;
                TRACE_J((&cpu), "jal");
                // Execute
                cpu.regFile[cpu.instFields.rd] = cpu.pc + 4;
                cpu.pc += cpu.targetAddress - 4;
                break;
            }
            default: { // Invalid instruction
                printf("[rISA]: Error - (0x%08x) is an invalid instruction.\n", cpu.IF);
                cpu.endTime = clock();
                cleanupSimulator(&cpu, EILSEQ);
            }
        }
        cpu.cycleCounter++;
        cpu.pc += 4;
        if (g_sigIntDet) { // Normal exit/cleanup
            cpu.endTime = clock();
            cleanupSimulator(&cpu, 0);
        }
        if (cpu.pc > cpu.virtMemSize) {
            printf("[rISA]: Error. Program counter is out of range.\n");
            cpu.endTime = clock();
            cleanupSimulator(&cpu, EFAULT);
        }
        if ((cpu.cycleCounter % cpu.intPeriodVal) == 0) {
            cpu.pfnIntHandler(&cpu);
        }
        cpu.regFile[0] = 0;
    }
    return 0;
}