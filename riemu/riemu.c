#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "riemu.h"

int main(void) {
    printf(
        "==============\n"
        "= RiEMU v0.1 =\n"
        "==============\n\n"
    );
    EMU_PRINTF("Init emulator...\n");

    // Init the PC, emu counter, virualized IO, memory, etc.
    u32 DummyMem[8] = { // TODO: Replace this later for a virtualized memory that emulates MMIO
                        // TODO: Memory needs to be byte-addressable...
        0x00a00113, /* addi x2, x0, 10  */
        0x00812703, /* lw x14, 8(x2)    */
        0x000111b7, /* lui x3, 17       */
        0x000023ef, /* jal x7, 2        */
        0x00e12423, /* sw x14, 8(x2)    */
        0x00a98863, /* beq x19, x10, 16 */
        0x00000033, /* add x0, x0, x0   */
        0x000000ff  /* invalid op check */
    };
    u32 pc = PC_START;
    u32 counter = 0;
    // TODO: Add virtualized IO setup later...
    // TODO: Add virtualized memory setup later...
    EMU_PRINTF("Init done.\n");
    EMU_PRINTF("Starting emulation...\n\n");

    // Run the emulator
    for (;;) {
        // Fetch
        u32 inst = DummyMem[pc];
        u8 opcode = inst & 0x7f;
        switch (OpcodeToFormat[opcode]) {
            case R: {
                // Decode
                u8 funct3 = (inst >> 12) & 0x7;
                u8 rd =     (inst >> 7)  & 0x1f;
                u8 rs1 =    (inst >> 15) & 0x1f;
                u8 rs2 =    (inst >> 20) & 0x1f;
                u8 funct7 = (inst >> 25) & 0x7f;
                EMU_PRINTF("Current instruction (0x%08x) is R-type\n",
                    inst);
                EMU_PRINTF("--- Fields ---\n"
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
                u8 funct3 = (inst >> 12) & 0x7;
                u8 rd =     (inst >> 7)  & 0x1f;
                u8 rs1 =    (inst >> 15) & 0x1f;
                u16 imm =   (inst >> 20) & 0xfff;
                EMU_PRINTF("Current instruction (0x%08x) is I-type\n",
                    inst);
                EMU_PRINTF("--- Fields ---\n"
                    "         funct3:    0x%01x\n"
                    "         rd:        0x%02x\n"
                    "         rs1:       0x%02x\n"
                    "         imm[11:0]: 0x%03x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)rd, (u32)rs1, (u32)imm
                );

                // Execute - TODO: Finish this...
                switch ((ItypeInstructions)((funct3 << 7) | (opcode))) {
                    case JALR:  { // Jump and link register
                        RegFile[rd] = ((pc+1) % 8); // TODO: PC needs to inc. by 4 once DummyMem is byte-addressable
                        pc = ((RegFile[rs1] + (((s32)imm << 20) >> 20) & 0xfffe) & 0xfffe) - 1; // TODO: <same-as-above-TODO>
                        break;
                    }
                    case LB:    {
                        
                    }
                    case LH:    {

                    }
                    case LW:    {

                    }
                    case LBU:   {
                        
                    }
                    case LHU:   {

                    }
                    case ADDI:  { // Add immediate
                        RegFile[rd] = RegFile[rs1] + (((s32)imm << 20) >> 20);
                        break;
                    }
                    case SLTI:  {

                    }
                    case SLTIU: {

                    }
                    case XORI:  {

                    }
                    case ORI:   {

                    }
                    case ANDI:  {

                    }
                }
                break;
            }
            case S: {
                // Decode
                u8 funct3 = (inst >> 12) & 0x7;
                u8 imm1 =   (inst >> 7)  & 0x1f;
                u8 rs1 =    (inst >> 15) & 0x1f;
                u8 rs2 =    (inst >> 20) & 0x1f;
                u16 imm2 =  (inst >> 25) & 0x7f;
                EMU_PRINTF("Current instruction (0x%08x) is S-type\n",
                    inst);
                EMU_PRINTF("--- Fields ---\n"
                    "         funct3:    0x%01x\n"
                    "         imm[4:0]:  0x%02x\n"
                    "         rs1:       0x%02x\n"
                    "         rs2:       0x%02x\n"
                    "         imm[11:5]: 0x%02x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)imm1, (u32)rs1, (u32)rs2, (u32)imm2
                );

                // Execute  - TODO: Finish this...
                switch ((StypeInstructions)((funct3 << 7) | (opcode))) {
                    case SB: {}
                    case SH: {}
                    case SW: {}
                }
                break;
            }
            case B: {
                // Decode
                // TODO: Pay attention to immediate bit ordering, fix this later...
                u8 funct3 = (inst >> 12) & 0x7;
                u8 imm1 =   (inst >> 7)  & 0x1f;
                u8 rs1 =    (inst >> 15) & 0x1f;
                u8 rs2 =    (inst >> 20) & 0x1f;
                u16 imm2 =  (inst >> 25) & 0x7f;
                EMU_PRINTF("Current instruction (0x%08x) is B-type\n",
                    inst);
                EMU_PRINTF("--- Fields ---\n"
                    "         funct3:       0x%01x\n"
                    "         imm[4:1|11]:  0x%02x\n"
                    "         rs1:          0x%02x\n"
                    "         rs2:          0x%02x\n"
                    "         imm[12|10:5]: 0x%02x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)imm1, (u32)rs1, (u32)rs2, (u32)imm2
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
                // TODO: Pay attention to immediate bit ordering, fix this later...
                u8 rd =   (inst >> 7)  & 0x1f;
                u32 imm = (inst >> 12) & 0xfffff;
                EMU_PRINTF("Current instruction (0x%08x) is U-type\n",
                    inst);
                EMU_PRINTF("--- Fields ---\n"
                    "         rd:     0x%02x\n"
                    "         imm:    0x%05x\n"
                    "         --------------\n",
                    (u32)rd, imm
                );

                // Execute  - TODO: Finish this...
                switch ((UtypeInstructions)(opcode)) {
                    case LUI:   {}
                    case AUIPC: {}
                }
                break;
            }
            case J: {
                // Decode
                // TODO: Pay attention to immediate bit ordering, fix this later...
                u8 rd =   (inst >> 7)  & 0x1f;
                u16 imm = (inst >> 12) & 0xfffff;
                EMU_PRINTF("Current instruction (0x%08x) is J-type\n",
                    inst);
                EMU_PRINTF("--- Fields ---\n"
                    "         rd:                    0x%02x\n"
                    "         imm[20|10:1|11|19:12]: 0x%05x\n"
                    "         --------------\n",
                    (u32)rd, imm
                );

                // Execute  - TODO: Finish this...
                switch ((JtypeInstructions)(opcode)) {
                    case JAL: { // Jump and link
                        RegFile[rd] = ((pc+1) % 8); // TODO: PC needs to inc. by 4 once DummyMem is byte-addressable
                        pc = (((((s32)imm << 20) >> 20) & 0xfffe) % 8) - 1; // TODO: <same-as-above-TODO>
                        break;
                    }
                }
                break;
            }
            default: {
                EMU_PRINTF("Uh-oh! (0x%08x) is an invalid instruction.\n",
                    inst);
                abort();
            }
        }
        
        // Update PC and counter; reset register x0 back to zero
        pc = (pc+1) % 8; // TODO: PC needs to inc. by 4 once DummyMem is byte-addressable
        counter++;       // TODO: Fetch counter update values based on instruction via some lookup table
        RegFile[0] = 0;
        EMU_PRINTF("<%d> cycle(s)\n", counter);
        
        // Temporary halt-on-each-clock-cycle functionality
        getc(stdin);
    }
    return 0;
}
