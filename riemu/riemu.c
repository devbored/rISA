#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "riemu.h"

int main(void) {
    DEBUG_PRINT(
        "Starting emulator.\n\n"
        "==============\n"
        "= RiEMU v0.1 =\n"
        "==============\n\n"
    );

    // Init the PC, emu counter, virualized IO, memory, etc.
    InstructionFields instBits;
    ImmediateFields immBits;
    u32 inst;
    s32 immFinal;
    u32 pc = PC_START;
    u32 cycleCounter = 0;
    u32 RegFile[32] = {0};
    u32 DummyMem[8] = { // TODO: Replace this later for a virtualized memory that emulates MMIO
        0x00a00113, /* addi x2, x0, 10  */
        0x00812703, /* lw x14, 8(x2)    */
        0x000111b7, /* lui x3, 17       */
        0x008003ef, /* jal x7, 8        */
        0x00e12423, /* sw x14, 8(x2)    */
        0xfedff3ef, /* jal x7, -20      */
        0x00000033, /* add x0, x0, x0   */
        0x000000ff  /* invalid op check */
    };
    const u32 memRange = sizeof(DummyMem) / sizeof(u32);

    // TODO: Add virtualized IO setup later...
    // TODO: Add virtualized memory setup later...
    DEBUG_PRINT("Init done.\n");
    DEBUG_PRINT("Starting emulator...\n\n");

    // Run the emulator
    for (;;) {
        // Fetch
        inst = DummyMem[pc];
        instBits.opcode = GET_BITSET(inst, 0, 7);
        switch (OpcodeToFormat[instBits.opcode]) {
            case R: {
                // Decode
                instBits.funct3 = GET_BITSET(inst, 12, 3);
                instBits.rd     = GET_BITSET(inst, 7, 5);
                instBits.rs1    = GET_BITSET(inst, 15, 5);
                instBits.rs2    = GET_BITSET(inst, 20, 5);
                instBits.funct7 = GET_BITSET(inst, 25, 7);
                DEBUG_PRINT("Current instruction (0x%08x) is R-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         funct3 : 0x%01x\n"
                    "         rd     : 0x%02x\n"
                    "         rs1    : 0x%02x\n"
                    "         rs2    : 0x%02x\n"
                    "         funct7 : 0x%02x\n"
                    "         --------------\n",
                    (u32)instBits.funct3,
                    (u32)instBits.rd,
                    (u32)instBits.rs1,
                    (u32)instBits.rs2,
                    (u32)instBits.funct7
                );

                // Execute
                switch ((RtypeInstructions)((instBits.funct7 << 10) | (instBits.funct3 << 7) | (instBits.opcode))) {
                    case SLLI: { // Shift left logical by immediate (i.e. rs2 is shamt)
                        RegFile[instBits.rd] = RegFile[instBits.rs1] << instBits.rs2; 
                        break; 
                    }
                    case SRLI: { // Shift right logical by immediate (i.e. rs2 is shamt)
                        RegFile[instBits.rd] = RegFile[instBits.rs1] >> instBits.rs2; 
                        break; 
                    }
                    case SRAI: { // Shift right arithmetic by immediate (i.e. rs2 is shamt)
                        RegFile[instBits.rd] = (s32)RegFile[instBits.rs1] >> instBits.rs2; 
                        break; 
                    }
                    case ADD:  { // Addition
                        RegFile[instBits.rd] = RegFile[instBits.rs1] + RegFile[instBits.rs2]; 
                        break; 
                    }
                    case SUB:  { // Subtraction
                        RegFile[instBits.rd] = RegFile[instBits.rs1] - RegFile[instBits.rs2]; 
                        break; 
                    }
                    case SLL:  { // Shift left logical
                        RegFile[instBits.rd] = RegFile[instBits.rs1] << RegFile[instBits.rs2]; 
                        break; 
                    }
                    case SLT:  { // Set if less than (signed)
                        RegFile[instBits.rd] = ((s32)RegFile[instBits.rs1] < (s32)RegFile[instBits.rs2]) ? (1) : (0); 
                        break; 
                    }
                    case SLTU: { // Set if less than (unsigned)
                        RegFile[instBits.rd] = (RegFile[instBits.rs1] < RegFile[instBits.rs2]) ? (1) : (0); 
                        break; 
                    }
                    case XOR:  { // Bitwise xor
                        RegFile[instBits.rd] = RegFile[instBits.rs1] ^ RegFile[instBits.rs2]; 
                        break; 
                    }
                    case SRL:  { // Shift right logical
                        RegFile[instBits.rd] = RegFile[instBits.rs1] >> RegFile[instBits.rs2]; 
                        break; 
                    }
                    case SRA:  { // Shift right arithmetic
                        RegFile[instBits.rd] = (s32)RegFile[instBits.rs1] >> RegFile[instBits.rs2]; 
                        break; 
                    }
                    case OR:   { // Bitwise or
                        RegFile[instBits.rd] = RegFile[instBits.rs1] | RegFile[instBits.rs2]; 
                        break; 
                    }
                    case AND:  { // Bitwise and
                        RegFile[instBits.rd] = RegFile[instBits.rs1] & RegFile[instBits.rs2]; 
                        break; 
                    }
                }
                break;
            }
            case I: {
                // Decode
                instBits.funct3   = GET_BITSET(inst, 12, 3);
                instBits.rd       = GET_BITSET(inst, 7, 5);
                instBits.rs1      = GET_BITSET(inst, 15, 5);
                immBits.imm11_0   = GET_BITSET(inst, 20, 12);
                immFinal = (((s32)immBits.imm11_0 << 20) >> 20);
                DEBUG_PRINT("Current instruction (0x%08x) is I-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         funct3   : 0x%01x\n"
                    "         rd       : 0x%02x\n"
                    "         rs1      : 0x%02x\n"
                    "         imm[11:0]: 0x%03x\n"
                    "         immFinal : 0x%08x\n"
                    "         --------------\n",
                    (u32)instBits.funct3,
                    (u32)instBits.rd,
                    (u32)instBits.rs1,
                    (u32)immBits.imm11_0,
                    (u32)immFinal
                );

                // Execute - TODO: Finish this...
                switch ((ItypeInstructions)((instBits.funct3 << 7) | (instBits.opcode))) {
                    case JALR:  { // Jump and link register
                        RegFile[instBits.rd] = ((pc + 1));
                        pc = (((RegFile[instBits.rs1] + immFinal) & 0xfffe) & 0xfffe) - 1; // TODO: <same-as-above-TODO>
                        break;
                    }
                    case LB:    { // Load byte (signed) - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case LH:    { // Load halfword (signed) - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case LW:    { // Load word
                        RegFile[instBits.rd] = DummyMem[RegFile[instBits.rs1] + immFinal];
                        break;
                    }
                    case LBU:   { // Load byte (unsigned) - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case LHU:   { // Load halfword (unsigned) - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case ADDI:  { // Add immediate
                        RegFile[instBits.rd] = RegFile[instBits.rs1] + immFinal;
                        break;
                    }
                    case SLTI:  { // Set if less than immediate (signed)
                        RegFile[instBits.rd] = ((s32)RegFile[instBits.rs1] < immFinal) ? (1) : (0);
                        break;
                    }
                    case SLTIU: {
                        RegFile[instBits.rd] = (RegFile[instBits.rs1] < (u32)immFinal) ? (1) : (0);
                        break;
                    }
                    case XORI:  { // Bitwise exclusive or immediate
                        RegFile[instBits.rd] = RegFile[instBits.rs1] ^ immFinal;
                        break;
                    }
                    case ORI:   { // Bitwise or immediate
                        RegFile[instBits.rd] = RegFile[instBits.rs1] | immFinal;
                        break;
                    }
                    case ANDI:  { // Bitwise and immediate
                        RegFile[instBits.rd] = RegFile[instBits.rs1] & immFinal;
                    }
                }
                break;
            }
            case S: {
                // Decode
                instBits.funct3   =  GET_BITSET(inst, 12, 3);
                immBits.imm4_0    =  GET_BITSET(inst, 7, 5);
                instBits.rs1      =  GET_BITSET(inst, 15, 5);
                instBits.rs2      =  GET_BITSET(inst, 20, 5);
                immBits.imm11_5   =  GET_BITSET(inst, 25, 7);
                immFinal = ((s32)((immBits.imm4_0 | (immBits.imm11_5 << 5)) << 20) >> 20);
                DEBUG_PRINT("Current instruction (0x%08x) is S-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         funct3   : 0x%01x\n"
                    "         imm[4:0] : 0x%02x\n"
                    "         rs1      : 0x%02x\n"
                    "         rs2      : 0x%02x\n"
                    "         imm[11:5]: 0x%02x\n"
                    "         immFinal : 0x%08x\n"
                    "         --------------\n",
                    (u32)instBits.funct3,
                    (u32)immBits.imm4_0,
                    (u32)instBits.rs1,
                    (u32)instBits.rs2,
                    (u32)immBits.imm11_5,
                    (u32)immFinal
                );

                // Execute  - TODO: Finish this...
                switch ((StypeInstructions)((instBits.funct3 << 7) | (instBits.opcode))) {
                    case SB: { // Store byte - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case SH: { // Store halfword - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case SW: { // Store word
                        DummyMem[RegFile[instBits.rs1] + immFinal] = RegFile[instBits.rs2];
                        break;
                    }
                }
                break;
            }
            case B: {
                // Decode
                instBits.funct3   = GET_BITSET(inst, 12, 3);
                instBits.rs1      = GET_BITSET(inst, 15, 5);
                instBits.rs2      = GET_BITSET(inst, 20, 5);
                immBits.imm11     = GET_BITSET(inst, 7, 1);
                immBits.imm4_1    = GET_BITSET(inst, 8, 4);
                immBits.imm10_5   = GET_BITSET(inst, 25, 6);
                immBits.imm12     = GET_BITSET(inst, 31, 1);
                immFinal = (immBits.imm11) | (immBits.imm4_1 << 1) | (immBits.imm10_5 << 5) | (immBits.imm12 << 11);
                immFinal = (s32)(immFinal << 20) >> 19;
                DEBUG_PRINT("Current instruction (0x%08x) is B-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         funct3      : 0x%01x\n"
                    "         imm[4:1|11] : 0x%02x\n"
                    "         rs1         : 0x%02x\n"
                    "         rs2         : 0x%02x\n"
                    "         imm[12|10:5]: 0x%02x\n"
                    "         immFinal    : 0x%08x\n"
                    "         --------------\n",
                    (u32)instBits.funct3,
                    (u32)((immBits.imm4_1 << 1) | (immBits.imm11)),
                    (u32)instBits.rs1,
                    (u32)instBits.rs2,
                    (u32)((immBits.imm12 << 6) | (immBits.imm10_5)),
                    (u32)immFinal
                );

                // Execute
                switch ((BtypeInstructions)((instBits.funct3 << 7) | (instBits.opcode))) {
                    case BEQ:  { // Branch if Equal
                        if ((s32)RegFile[instBits.rs1] == (s32)RegFile[instBits.rs2]) {
                            pc += ((immFinal / 8)) - 1;
                        }
                        break;
                    }
                    case BNE:  { // Branch if Not Equal
                        if ((s32)RegFile[instBits.rs1] != (s32)RegFile[instBits.rs2]) {
                            pc += ((immFinal / 8)) - 1;
                        }
                        break;
                    }
                    case BLT:  { // Branch if Less Than
                        if ((s32)RegFile[instBits.rs1] < (s32)RegFile[instBits.rs2]) {
                            pc += ((immFinal / 8)) - 1;
                        }
                        break;
                    }
                    case BGE:  { // Branch if Greater Than or Equal
                        if ((s32)RegFile[instBits.rs1] >= (s32)RegFile[instBits.rs2]) {
                            pc += ((immFinal / 8)) - 1;
                        }
                        break;
                    }
                    case BLTU: { // Branch if Less Than (unsigned)
                        if (RegFile[instBits.rs1] < RegFile[instBits.rs2]) {
                            pc += ((immFinal / 8)) - 1;
                        }
                        break;
                    }
                    case BGEU: { // Branch if Greater Than or Equal (unsigned)
                        if (RegFile[instBits.rs1] >= RegFile[instBits.rs2]) {
                            pc += ((immFinal / 8)) - 1;
                        }
                        break;
                    }
                }
                break;
            }
            case U: {
                // Decode
                instBits.rd      = GET_BITSET(inst, 7, 5);
                immBits.imm31_12 = GET_BITSET(inst, 12, 20);
                immFinal = immBits.imm31_12 << 12;
                DEBUG_PRINT("Current instruction (0x%08x) is U-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         rd         : 0x%02x\n"
                    "         imm[31:12] : 0x%05x\n"
                    "         immFinal   : 0x%08x\n"
                    "         --------------\n",
                    (u32)instBits.rd,
                    (u32)immBits.imm31_12,
                    (u32)immFinal
                );

                // Execute
                switch ((UtypeInstructions)(instBits.opcode)) {
                    case LUI:   { // Load Upper Immediate
                        RegFile[instBits.rd] = immFinal;
                        break;
                    }
                    case AUIPC: { // Add Upper Immediate to PC
                        RegFile[instBits.rd] = pc + immFinal;
                    }
                }
                break;
            }
            case J: {
                // Decode
                instBits.rd       = GET_BITSET(inst, 7, 5);
                immBits.imm19_12  = GET_BITSET(inst, 12, 8);
                immBits.imm11     = GET_BITSET(inst, 20, 1);
                immBits.imm10_1   = GET_BITSET(inst, 21, 10);
                immBits.imm20     = GET_BITSET(inst, 31, 1);
                immFinal = (immBits.imm10_1) | (immBits.imm11 << 10) | (immBits.imm19_12 << 11) | (immBits.imm20 << 19);
                immFinal = (s32)(immFinal << 12) >> 11;
                DEBUG_PRINT("Current instruction (0x%08x) is J-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         rd         : 0x%02x\n"
                    "         imm[20]    : 0x%01x\n"
                    "         imm[10:1]  : 0x%03x\n"
                    "         imm[11]    : 0x%01x\n"
                    "         imm[19:12] : 0x%02x\n"
                    "         immFinal   : 0x%05x\n"
                    "         --------------\n",
                    (u32)instBits.rd,
                    (u32)immBits.imm20,
                    (u32)immBits.imm10_1,
                    (u32)immBits.imm11,
                    (u32)immBits.imm19_12,
                    (u32)immFinal
                );

                // Execute
                switch ((JtypeInstructions)(instBits.opcode)) {
                    case JAL: { // Jump and link
                        RegFile[instBits.rd] = ((pc + 1));
                        pc += (immFinal / 4) - 1;
                        break;
                    }
                }
                break;
            }
            default: { // Invalid instruction
                DEBUG_PRINT("Error. (0x%08x) is an invalid instruction.\n", inst);
                abort();
            }
        }

        // Update PC and counter, reset register x0 back to zero
        pc += 1;
        if (pc > memRange) {
            DEBUG_PRINT("Error. Program counter is out of range.\n");
            abort();
        }
        cycleCounter++; // TODO: Fetch counter update values based on instruction via some lookup table
        RegFile[0] = 0;
        DEBUG_PRINT("<%d> cycle(s)\n", cycleCounter);
        if (cycleCounter == INT32_MAX) {
            DEBUG_PRINT("Emulator timeout reached. Exiting...\n");
            return 0;
        }
		//getc(stdin);
    }
    return 0;
}
