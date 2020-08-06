#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "riemu.h"

// Test 32B memory for now
// TODO: Replace this later for a virtualized memory that emulates MMIO
u32 DummyMem[8] = {
    0x00a00113, /* addi x2, x0, 10  */
    0x00812703, /* lw x14, 8(x2)    */
    0x000111b7, /* lui x3, 17       */
    0x0000f3ef, /* jal x7, 15       */
    0x00e12423, /* sw x14, 8(x2)    */
    0x00a98863, /* beq x19, x10, 16 */
    0x00000033, /* add x0, x0, x0   */
    0x000000ff  /* invalid op check */
};

int main(void) {
    printf(
        "==============\n"
        "= RiEMU v0.1 =\n"
        "==============\n\n"
    );
    EMU_PRINTF("Init emulator...\n");

    // Init the PC, emu counter, virualized IO, and memory
    u32 pc = PC_START;
    u32 counter = 0;
    // TODO: Add virtualized IO setup later...
    // TODO: Add virtualized memory setup later...
    EMU_PRINTF("Init done.\n");
    EMU_PRINTF("Starting emulation...\n\n");

    // Run the emulator
    for(;;) {
        switch(OpcodeToFormat[DummyMem[pc] & 0x7f]) {
            case R: {
                // Fetch-and-decode
                u8 funct3 = (DummyMem[pc] >> 12) & 0x7;
                u8 rd = (DummyMem[pc] >> 7) & 0x1f;
                u8 rs1 = (DummyMem[pc] >> 15) & 0x1f;
                u8 rs2 = (DummyMem[pc] >> 20) & 0x1f;
                u8 funct7 = (DummyMem[pc] >> 25) & 0x7f;
                EMU_PRINTF("Current instruction (0x%08x) is R-type\n",
                    DummyMem[pc]);
                EMU_PRINTF("--- Fields ---\n"
                    "         funct3: 0x%01x\n"
                    "         rd:     0x%02x\n"
                    "         rs1:    0x%02x\n"
                    "         rs2:    0x%02x\n"
                    "         funct7: 0x%02x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)rd, (u32)rs1, (u32)rs2, (u32)funct7
                );
                break;
            }
            case I: {
                // Fetch-and-decode
                u8 funct3 = (DummyMem[pc] >> 12) & 0x7;
                u8 rd = (DummyMem[pc] >> 7) & 0x1f;
                u8 rs1 = (DummyMem[pc] >> 15) & 0x1f;
                u16 imm = (DummyMem[pc] >> 20) & 0xfff;
                EMU_PRINTF("Current instruction (0x%08x) is I-type\n",
                    DummyMem[pc]);
                EMU_PRINTF("--- Fields ---\n"
                    "         funct3:    0x%01x\n"
                    "         rd:        0x%02x\n"
                    "         rs1:       0x%02x\n"
                    "         imm[11:0]: 0x%03x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)rd, (u32)rs1, (u32)imm
                );
                break;
            }
            case S: {
                // Fetch-and-decode
                u8 funct3 = (DummyMem[pc] >> 12) & 0x7;
                u8 imm1 = (DummyMem[pc] >> 7) & 0x1f;
                u8 rs1 = (DummyMem[pc] >> 15) & 0x1f;
                u8 rs2 = (DummyMem[pc] >> 20) & 0x1f;
                u16 imm2 = (DummyMem[pc] >> 25) & 0x7f;
                EMU_PRINTF("Current instruction (0x%08x) is S-type\n",
                    DummyMem[pc]);
                EMU_PRINTF("--- Fields ---\n"
                    "         funct3:    0x%01x\n"
                    "         imm[4:0]:  0x%02x\n"
                    "         rs1:       0x%02x\n"
                    "         rs2:       0x%02x\n"
                    "         imm[11:5]: 0x%02x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)imm1, (u32)rs1, (u32)rs2, (u32)imm2
                );
                break;
            }
            case B: {
                // Fetch-and-decode
                // TODO: Pay attention to immediate bit ordering, fix this later...
                u8 funct3 = (DummyMem[pc] >> 12) & 0x7;
                u8 imm1 = (DummyMem[pc] >> 7) & 0x1f;
                u8 rs1 = (DummyMem[pc] >> 15) & 0x1f;
                u8 rs2 = (DummyMem[pc] >> 20) & 0x1f;
                u16 imm2 = (DummyMem[pc] >> 25) & 0x7f;
                EMU_PRINTF("Current instruction (0x%08x) is B-type\n",
                    DummyMem[pc]);
                EMU_PRINTF("--- Fields ---\n"
                    "         funct3:       0x%01x\n"
                    "         imm[4:1|11]:  0x%02x\n"
                    "         rs1:          0x%02x\n"
                    "         rs2:          0x%02x\n"
                    "         imm[12|10:5]: 0x%02x\n"
                    "         --------------\n",
                    (u32)funct3, (u32)imm1, (u32)rs1, (u32)rs2, (u32)imm2
                );
                break;
            }
            case U: {
                // Fetch-and-decode
                // TODO: Pay attention to immediate bit ordering, fix this later...
                u8 rd = (DummyMem[pc] >> 7) & 0x1f;
                u32 imm = (DummyMem[pc] >> 12) & 0xfffff;
                EMU_PRINTF("Current instruction (0x%08x) is U-type\n",
                    DummyMem[pc]);
                EMU_PRINTF("--- Fields ---\n"
                    "         rd:     0x%02x\n"
                    "         imm:    0x%05x\n"
                    "         --------------\n",
                    (u32)rd, imm
                );
                break;
            }
            case J: {
                // Fetch-and-decode
                // TODO: Pay attention to immediate bit ordering, fix this later...
                u8 rd = (DummyMem[pc] >> 7) & 0x1f;
                u16 imm = (DummyMem[pc] >> 12) & 0xfffff;
                EMU_PRINTF("Current instruction (0x%08x) is J-type\n",
                    DummyMem[pc]);
                EMU_PRINTF("--- Fields ---\n"
                    "         rd:                    0x%02x\n"
                    "         imm[20|10:1|11|19:12]: 0x%05x\n"
                    "         --------------\n",
                    (u32)rd, imm
                );
                break;
            }
            default: {
                EMU_PRINTF("Uh-oh! (0x%08x) is an invalid instruction.\n",
                    DummyMem[pc]);
                abort();
            }
        }
        // Update PC and counter
        pc = (pc+1) % 8;
        counter++;
        EMU_PRINTF("<%d> cycle(s)\n", counter);
        
        // Temp halt-on-each-clock-cycle functionality
        getc(stdin);
    }
    return 0;
}
