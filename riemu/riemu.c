#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "riemu.h"

int main(void) {
    DEBUG_PRINT(
        "Starting emulator.\n\n"
        "==============\n"
        "= RiEMU v0.1 =\n"
        "==============\n\n"
    );

    // Init the PC, emu counter, virualized IO, memory, etc.
    u32 DummyMem[8] = { // TODO: Replace this later for a virtualized memory that emulates MMIO
                        // TODO: Memory needs to be byte-addressable...
        0x00a00113, /* addi x2, x0, 10  */
        0x00812703, /* lw x14, 8(x2)    */
        0x000111b7, /* lui x3, 17       */
        0x000003ef, /* jal x7, 2        */
        0x00e12423, /* sw x14, 8(x2)    */
        0x00a99463, /* beq x19, x10, 16 */
        0x00000033, /* add x0, x0, x0   */
        0x000000ff  /* invalid op check */
    };
    u32 pc = PC_START;
    u32 counter = 0;
    // TODO: Add virtualized IO setup later...
    // TODO: Add virtualized memory setup later...
    DEBUG_PRINT("Init done.\n");
    DEBUG_PRINT("Starting emulation...\n\n");

    // Run the emulator
    for (;;) {
        // Fetch
        u32 inst = DummyMem[pc];
        u8 opcode = inst & 0x7f;
        switch (OpcodeToFormat[opcode]) {
            case R: {
                // Decode
                u8 funct3 = (inst >> 12) & 0x7;
                u8 rd     = (inst >> 7)  & 0x1f;
                u8 rs1    = (inst >> 15) & 0x1f;
                u8 rs2    = (inst >> 20) & 0x1f;
                u8 funct7 = (inst >> 25) & 0x7f;
                DEBUG_PRINT("Current instruction (0x%08x) is R-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         funct3: 0x%01x\n"
                    "         rd:     0x%02x\n"
                    "         rs1:    0x%02x\n"
                    "         rs2:    0x%02x\n"
                    "         funct7: 0x%02x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)rd, (u32)rs1, (u32)rs2, (u32)funct7
                );

                // Execute
                switch ((RtypeInstructions)((funct7 << 10) | (funct3 << 7) | (opcode))) {
                    case SLLI: { // Shift left logical by immediate (i.e. rs2 is shamt)
                        RegFile[rd] = RegFile[rs1] << rs2; 
                        break; 
                    }
                    case SRLI: { // Shift right logical by immediate (i.e. rs2 is shamt)
                        RegFile[rd] = RegFile[rs1] >> rs2; 
                        break; 
                    }
                    case SRAI: { // Shift right arithmetic by immediate (i.e. rs2 is shamt)
                        RegFile[rd] = (s32)RegFile[rs1] >> rs2; 
                        break; 
                    }
                    case ADD:  { // Addition
                        RegFile[rd] = RegFile[rs1] + RegFile[rs2]; 
                        break; 
                    }
                    case SUB:  { // Subtraction
                        RegFile[rd] = RegFile[rs1] - RegFile[rs2]; 
                        break; 
                    }
                    case SLL:  { // Shift left logical
                        RegFile[rd] = RegFile[rs1] << RegFile[rs2]; 
                        break; 
                    }
                    case SLT:  { // Set if less than (signed)
                        RegFile[rd] = ((s32)RegFile[rs1] < (s32)RegFile[rs2]) ? (1) : (0); 
                        break; 
                    }
                    case SLTU: { // Set if less than (unsigned)
                        RegFile[rd] = (RegFile[rs1] < RegFile[rs2]) ? (1) : (0); 
                        break; 
                    }
                    case XOR:  { // Bitwise xor
                        RegFile[rd] = RegFile[rs1] ^ RegFile[rs2]; 
                        break; 
                    }
                    case SRL:  { // Shift right logical
                        RegFile[rd] = RegFile[rs1] >> RegFile[rs2]; 
                        break; 
                    }
                    case SRA:  { // Shift right arithmetic
                        RegFile[rd] = (s32)RegFile[rs1] >> RegFile[rs2]; 
                        break; 
                    }
                    case OR:   { // Bitwise or
                        RegFile[rd] = RegFile[rs1] | RegFile[rs2]; 
                        break; 
                    }
                    case AND:  { // Bitwise and
                        RegFile[rd] = RegFile[rs1] & RegFile[rs2]; 
                        break; 
                    }
                }
                break;
            }
            case I: {
                // Decode
                u8 funct3    = (inst >> 12) & 0x7;
                u8 rd        = (inst >> 7)  & 0x1f;
                u8 rs1       = (inst >> 15) & 0x1f;
                u16 imm      = (inst >> 20) & 0xfff;
                s32 immFinal = (((s32)imm << 20) >> 20);
                DEBUG_PRINT("Current instruction (0x%08x) is I-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         funct3:    0x%01x\n"
                    "         rd:        0x%02x\n"
                    "         rs1:       0x%02x\n"
                    "         imm[11:0]: 0x%03x\n"
                    "         immFinal:  0x%08x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)rd, (u32)rs1, (u32)imm, (u32)immFinal
                );

                // Execute - TODO: Finish this...
                switch ((ItypeInstructions)((funct3 << 7) | (opcode))) {
                    case JALR:  { // Jump and link register
                        RegFile[rd] = ((pc+1) % 8); // TODO: PC needs to inc. by 4 once DummyMem is byte-addressable
                        pc = (((RegFile[rs1] + immFinal) & 0xfffe) & 0xfffe) - 1; // TODO: <same-as-above-TODO>
                        break;
                    }
                    case LB:    { // Load byte (signed) - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case LH:    { // Load halfword (signed) - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case LW:    { // Load word
                        RegFile[rd] = DummyMem[RegFile[rs1] + immFinal];
                        break;
                    }
                    case LBU:   { // Load byte (unsigned) - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case LHU:   { // Load halfword (unsigned) - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case ADDI:  { // Add immediate
                        RegFile[rd] = RegFile[rs1] + immFinal;
                        break;
                    }
                    case SLTI:  { // Set if less than immediate (signed)
                        RegFile[rd] = ((s32)RegFile[rs1] < immFinal) ? (1) : (0);
                        break;
                    }
                    case SLTIU: {
                        RegFile[rd] = (RegFile[rs1] < (u32)immFinal) ? (1) : (0);
                        break;
                    }
                    case XORI:  { // Bitwise exclusive or immediate
                        RegFile[rd] = RegFile[rs1] ^ immFinal;
                        break;
                    }
                    case ORI:   { // Bitwise or immediate
                        RegFile[rd] = RegFile[rs1] | immFinal;
                        break;
                    }
                    case ANDI:  { // Bitwise and immediate
                        RegFile[rd] = RegFile[rs1] & immFinal;
                    }
                }
                break;
            }
            case S: {
                // Decode
                u8 funct3    = (inst >> 12) & 0x7;
                u8 imm1      = (inst >> 7)  & 0x1f;
                u8 rs1       = (inst >> 15) & 0x1f;
                u8 rs2       = (inst >> 20) & 0x1f;
                u16 imm2     = (inst >> 25) & 0x7f;
                s32 immFinal = (((s32)((imm2 << 5) | imm1) << 20) >> 20);
                DEBUG_PRINT("Current instruction (0x%08x) is S-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         funct3:    0x%01x\n"
                    "         imm[4:0]:  0x%02x\n"
                    "         rs1:       0x%02x\n"
                    "         rs2:       0x%02x\n"
                    "         imm[11:5]: 0x%02x\n"
                    "         immFinal:  0x%08x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)imm1, (u32)rs1, (u32)rs2, (u32)imm2, (u32)immFinal
                );

                // Execute  - TODO: Finish this...
                switch ((StypeInstructions)((funct3 << 7) | (opcode))) {
                    case SB: { // Store byte - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case SH: { // Store halfword - TODO: Blocked until memory can be made byte addressable
                        break;
                    }
                    case SW: { // Store word
                        DummyMem[RegFile[rs1] + immFinal] = RegFile[rs2];
                        break;
                    }
                }
                break;
            }
            case B: {
                // Decode
                u8 funct3    = (inst >> 12) & 0x7;
                u8 imm1      = (inst >> 7)  & 0x1f;
                u8 rs1       = (inst >> 15) & 0x1f;
                u8 rs2       = (inst >> 20) & 0x1f;
                u16 imm2     = (inst >> 25) & 0x7f;
                u16 imm      = ((imm2 << 5) | imm1);
                s32 immFinal = (((s32)((imm & 0x800) | ((imm & 0x1) << 10) | ((imm & 0x7e0) >> 1) | (imm & 0x1e) >> 1) << 12) >> 11);
                DEBUG_PRINT("Current instruction (0x%08x) is B-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         funct3:       0x%01x\n"
                    "         imm[4:1|11]:  0x%02x\n"
                    "         rs1:          0x%02x\n"
                    "         rs2:          0x%02x\n"
                    "         imm[12|10:5]: 0x%02x\n"
                    "         immFinal:     0x%08x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)imm1, (u32)rs1, (u32)rs2, (u32)imm2, (u32)immFinal
                );

                // Execute  - TODO: Finish this...
                switch ((BtypeInstructions)((funct3 << 7) | (opcode))) {
                    case BEQ:  {}
                    case BNE:  {}
                    case BLT:  {}
                    case BGE:  {}
                    case BLTU: {}
                    case BGEU: {}
                }
                break;
            }
            case U: {
                // Decode
                u8 rd        = (inst >> 7)  & 0x1f;
                u32 imm      = (inst >> 12) & 0xfffff;
                s32 immFinal = (s32)(imm << 12);
                DEBUG_PRINT("Current instruction (0x%08x) is U-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         rd:       0x%02x\n"
                    "         imm:      0x%05x\n"
                    "         immFinal: 0x%08x\n"
                    "         --------------\n",
                    (u32)rd, (u32)imm, (u32)immFinal
                );

                // Execute
                switch ((UtypeInstructions)(opcode)) {
                    case LUI:   {
                        RegFile[rd] = immFinal;
                        break;
                    }
                    case AUIPC: {
                        RegFile[rd] = pc + immFinal;
                    }
                }
                break;
            }
            case J: {
                // Decode
                u8 rd        = (inst >> 7)  & 0x1f;
                u32 imm      = (inst >> 12) & 0xfffff;
                s32 immFinal = (((s32)((imm & 0x40000) | ((imm & 0xff) << 11) | ((imm & 0x100) << 2) | (imm & 0x7fe00)) << 12) >> 11);
                DEBUG_PRINT("Current instruction (0x%08x) is J-type\n", inst);
                DEBUG_PRINT("--- Fields ---\n"
                    "         rd:                    0x%02x\n"
                    "         imm[20|10:1|11|19:12]: 0x%05x\n"
                    "         immFinal:              0x%08x\n"
                    "         --------------\n",
                    (u32)rd, (u32)imm, (u32)immFinal
                );

                // Execute  - TODO: Finish this...
                switch ((JtypeInstructions)(opcode)) {
                    case JAL: { // Jump and link
                        RegFile[rd] = ((pc+1) % 8); // TODO: PC needs to inc. by 4 once DummyMem is byte-addressable
                        pc = (immFinal % 8) - 1; // TODO: <same-as-above-TODO>
                        break;
                    }
                }
                break;
            }
            default: {
                DEBUG_PRINT("Uh-oh! (0x%08x) is an invalid instruction.\n", inst);
                abort();
            }
        }
        
        // Update PC and counter; reset register x0 back to zero
        pc = (pc+1) % 8; // TODO: PC needs to inc. by 4 once DummyMem is byte-addressable
        counter++;       // TODO: Fetch counter update values based on instruction via some lookup table
        RegFile[0] = 0;
        DEBUG_PRINT("<%d> cycle(s)\n", counter);
        
        // Temporary halt-on-each-clock-cycle functionality
        getc(stdin);
    }
    return 0;
}
