#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "risa.h"
#include "gdbserver.h"

static volatile int g_sigIntDet = 0;
static SIGINT_RET_TYPE sigintHandler(SIGINT_PARAM sig) {
    g_sigIntDet = 1;
    SIGINT_RET;
}

void defaultMmioHandler(rv32iHart *cpu)  { return; }
void defaultIntHandler(rv32iHart *cpu)   { return; }
void defaultEnvHandler(rv32iHart *cpu)   { return; }
void defaultExitHandler(rv32iHart *cpu)  { return; }
void defaultInitHandler(rv32iHart *cpu)  { return; }

void printHelp(void) {
    printf(
        "[rISA]:    Usage: risa [OPTIONS] <program_binary>\n"
        "           Example: risa -m 1024 my_riscv_program.hex"
        "\n\n"
        "           OPTIONS:\n"
        "               -m <uint> : Virtual memory/IO size (in bytes).\n"
        "               -l <file> : Dynamic library '.so/.dll' file to user-defined handler functions.\n"
        "               -t        : Enable trace printing to stderr.\n"
        "               -i <uint> : Simulator interrupt-check timeout value [DEFAULT=500].\n"
        "               -gdb      : Run the simulator in GDB-mode.\n"
        "               -h        : Print help and exit.\n"
        "\n"
    );
}

void cleanupSimulator(rv32iHart *cpu) {
    if (cpu->pfnExitHandler != NULL)    { cpu->pfnExitHandler(cpu);    }
    if (cpu->virtMem        != NULL)    { free(cpu->virtMem);          }
    if (cpu->handlerData    != NULL)    { free(cpu->handlerData);      }
    if (cpu->handlerLib     != NULL)    { CLOSE_LIB(cpu->handlerLib);  }
    printf(
        "[rISA]: Simulator stopping.\n"
        "        Simulation time elapsed: (%f seconds).\n",
        ((double)(cpu->endTime - cpu->startTime)) / CLOCKS_PER_SEC
    );
}

SimulatorOptions isOption(const char *arg) {
    if      (strcmp(arg, "-m")      == 0)   { return OPT_VIRT_MEM_SIZE; }
    else if (strcmp(arg, "-l")      == 0)   { return OPT_HANDLER_LIB;   }
    else if (strcmp(arg, "-h")      == 0)   { return OPT_HELP;          }
    else if (strcmp(arg, "-t")      == 0)   { return OPT_TRACING;       }
    else if (strcmp(arg, "-i")      == 0)   { return OPT_INTERRUPT;     }
    else if (strcmp(arg, "-gdb")    == 0)   { return OPT_GDB;           }
    else                                    { return OPT_UNKNOWN;       }
}

int processOptions(int argc, char **argv, rv32iHart *cpu) {
    for (int i=1; i<argc; ++i) {
        if (((i+1) != argc) && (isOption(argv[i]) & VALUE_OPTS) && (isOption(argv[i+1]) & VALUE_OPTS)) {
            printf("[rISA]: ERROR - Invalid formatting of options.\n");
            printHelp();
            cleanupSimulator(cpu);
            return EINVAL;
        }
        if (((i+1) == argc) && (isOption(argv[i]) & VALUE_OPTS)) {
            printf("[rISA]: ERROR - Invalid formatting of options.\n");
            printHelp();
            cleanupSimulator(cpu);
            return EINVAL;
        }
        switch (isOption(argv[i])) {
            case OPT_VIRT_MEM_SIZE: {
                if (cpu->opts.o_virtMemSize) {
                    printf("[rISA]: ERROR - Ambiguous multiple '-m' options defined - specify only one.\n");
                    printHelp();
                    cleanupSimulator(cpu);
                    return EINVAL;
                }
                int size = (u32)atoi(argv[i+1]);
                cpu->virtMemSize = size;
                cpu->opts.o_virtMemSize = 1;
                break;
            }
            case OPT_HANDLER_LIB: {
                if (cpu->opts.o_definedHandles) {
                    printf("[rISA]: ERROR - Ambiguous multiple '-l' options defined (specify only one).\n");
                    printHelp();
                    cleanupSimulator(cpu);
                    return EINVAL;
                }
                cpu->handlerLib = LOAD_LIB(argv[i+1]);
                if (cpu->handlerLib == NULL) {
                    printf(
                        "[rISA]: INFO - Could not load dynamic library <%s>.\n"
                        "        Using default default handlers instead.\n", argv[i+1]
                    );
                }
                cpu->pfnMmioHandler = (pfnMmioHandler)LOAD_SYM(cpu->handlerLib, "risaMmioHandler");
                cpu->pfnIntHandler  = (pfnIntHandler)LOAD_SYM(cpu->handlerLib,  "risaIntHandler");
                cpu->pfnEnvHandler  = (pfnEnvHandler)LOAD_SYM(cpu->handlerLib,  "risaEnvHandler");
                cpu->pfnInitHandler = (pfnInitHandler)LOAD_SYM(cpu->handlerLib, "risaInitHandler");
                cpu->pfnExitHandler = (pfnExitHandler)LOAD_SYM(cpu->handlerLib, "risaExitHandler");
                cpu->opts.o_definedHandles = 1;
                break;
            }
            case OPT_HELP:
                printHelp();
                cleanupSimulator(cpu);
                exit(0);
            case OPT_TRACING:
                cpu->opts.o_tracePrintEnable = 1;
                break;
            case OPT_INTERRUPT: {
                if (cpu->opts.o_intPeriod) {
                    printf("[rISA]: ERROR - Ambiguous multiple '-i' options defined (specify only one).\n");
                    printHelp();
                    cleanupSimulator(cpu);
                    return EINVAL;
                }
                cpu->intPeriodVal = (u32)atoi(argv[i+1]);
                cpu->opts.o_intPeriod = 1;
                break;
            }
            case OPT_GDB: {
                cpu->opts.o_gdbEnabled = 1;
                break;
            }
            default: // --- Unknown value if prior arg was not a valid OPTION <value> ---
                if ((i != argc-1) && !(isOption(argv[i-1]) & VALUE_OPTS)) {
                    printf("[rISA]: ERROR - Unknown option <%s>.\n", argv[i]);
                    printHelp();
                    cleanupSimulator(cpu);
                    return EINVAL;
                }
            break;
        }
    }
    return 0;
}

int loadProgram(int argc, char **argv, rv32iHart *cpu) {
    if ((isOption(argv[argc-1]) != OPT_UNKNOWN) || (isOption(argv[argc-2]) == VALUE_OPTS)) {
        printf("[rISA]: ERROR - No program specified.\n");
        printHelp();
        cleanupSimulator(cpu);
        return EINVAL;
    }
    FILE* binFile;
    OPEN_FILE(binFile, argv[argc-1], "rb");
    if (binFile == NULL) {
        printf("[rISA]: ERROR - Could not open file <%s>.\n", argv[argc-1]);
        printHelp();
        cleanupSimulator(cpu);
        return EIO;
    }
    for (int i=0; !feof(binFile) != 0; ++i) {
        if (i >= (cpu->virtMemSize / sizeof(u32))) {
            printf("[rISA]: ERROR - Could not fit <%s> in simulator's virtual memory.\n", argv[argc-1]);
            fclose(binFile);
            cleanupSimulator(cpu);
            return ENOMEM;
        }
        fread(cpu->virtMem+i, sizeof(u32), 1, binFile);
    }
    fclose(binFile);
    return 0;
}

int setupSimulator(int argc, char **argv, rv32iHart *cpu) {
    int err;
    if (argc == 1) {
        printf("[rISA]: ERROR - No program specified.\n");
        printHelp();
        cleanupSimulator(cpu);
        return EINVAL;
    }

    err = processOptions(argc, argv, cpu);
    if (err) {
        return err;
    }

    if (cpu->pfnMmioHandler == NULL) {
        cpu->pfnMmioHandler = defaultMmioHandler;
        printf("[rISA]: INFO - 'risaMmioHandler' not found in handler lib, using default default instead.\n");
    }
    if (cpu->pfnIntHandler == NULL) {
        cpu->pfnIntHandler  = defaultIntHandler;
        printf("[rISA]: INFO - 'risaIntHandle' not found in handler lib, using default default instead.\n");
    }
    if (cpu->pfnEnvHandler == NULL) {
        cpu->pfnEnvHandler  = defaultEnvHandler;
        printf("[rISA]: INFO - 'risaEnvHandler' not found in handler lib, using default default instead.\n");
    }
    if (cpu->pfnInitHandler == NULL) {
        cpu->pfnInitHandler = defaultInitHandler;
        printf("[rISA]: INFO - 'risaInitHandler' not found in handler lib, using default default instead.\n");
    }
    if (cpu->pfnExitHandler == NULL) {
        cpu->pfnExitHandler = defaultExitHandler;
        printf("[rISA]: INFO - 'risaExitHandler' not found in handler lib, using default default instead.\n");
    }
    if (cpu->intPeriodVal == 0) {
        cpu->intPeriodVal = DEFAULT_INT_PERIOD;
        printf("[rISA]: INFO - Interrupt period cannot be 0, using default value '%d' instead.\n",
            DEFAULT_INT_PERIOD);
    }
    if (cpu->virtMemSize == 0) {
        cpu->virtMemSize = DEFAULT_VIRT_MEM_SIZE;
        printf("[rISA]: INFO - Virtual memory size cannot be 0, using default value '%d' instead.\n",
            DEFAULT_VIRT_MEM_SIZE);
    }

    if (cpu->opts.o_gdbEnabled) {
        gdbserverInit(cpu);
    }

    cpu->virtMem = malloc(cpu->virtMemSize);
    if (cpu->virtMem == NULL) {
        printf("[rISA]: ERROR - Could not allocate virtual memory.\n");
        cleanupSimulator(cpu);
        return ENOMEM;
    }
    err = loadProgram(argc, argv, cpu);
    if (err) {
        return err;
    }

    // Grab the end-of-stack-pointer address
    cpu->regFile[SP] = cpu->virtMem[SP_ADDR_OFFSET];

    // Init the PC value
    cpu->pc = cpu->virtMem[START_PC_ADDR_OFFSET];

    cpu->pfnInitHandler(cpu);
    return 0;
}

int executionLoop(rv32iHart *cpu) {
    cpu->startTime = clock();
    SIGINT_REGISTER(cpu, sigintHandler);
    for (;;) {
        // Process GDB commands
        if (cpu->opts.o_gdbEnabled) {
            gdbserverCall(cpu);
        }
        // Fetch
        cpu->IF = ACCESS_MEM_W(cpu->virtMem, cpu->pc);
        cpu->instFields.opcode = GET_OPCODE(cpu->IF);
        switch (g_opcodeToFormat[cpu->instFields.opcode]) {
            case R: {
                // Decode
                cpu->instFields.rd     = GET_RD(cpu->IF);
                cpu->instFields.rs1    = GET_RS1(cpu->IF);
                cpu->instFields.rs2    = GET_RS2(cpu->IF);
                cpu->instFields.funct3 = GET_FUNCT3(cpu->IF);
                cpu->instFields.funct7 = GET_FUNCT7(cpu->IF);
                cpu->ID = (cpu->instFields.funct7 << 10) | (cpu->instFields.funct3 << 7) | cpu->instFields.opcode;
                // Execute
                switch ((RtypeInstructions)cpu->ID) {
                    case SLLI: { // Shift left logical by immediate (i.e. rs2 is shamt)
                        TRACE_R((cpu), "slli");
                        cpu->regFile[cpu->instFields.rd] = cpu->regFile[cpu->instFields.rs1] << cpu->instFields.rs2;
                        break;
                    }
                    case SRLI: { // Shift right logical by immediate (i.e. rs2 is shamt)
                        TRACE_R((cpu), "srli");
                        cpu->regFile[cpu->instFields.rd] = cpu->regFile[cpu->instFields.rs1] >> cpu->instFields.rs2;
                        break;
                    }
                    case SRAI: { // Shift right arithmetic by immediate (i.e. rs2 is shamt)
                        TRACE_R((cpu), "srai");
                        cpu->regFile[cpu->instFields.rd] =
                            (u32)((s32)cpu->regFile[cpu->instFields.rs1] >> cpu->instFields.rs2);
                        break;
                    }
                    case ADD:  { // Addition
                        TRACE_R((cpu), "add");
                        cpu->regFile[cpu->instFields.rd] =
                            cpu->regFile[cpu->instFields.rs1] + cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                    case SUB:  { // Subtraction
                        TRACE_R((cpu), "sub");
                        cpu->regFile[cpu->instFields.rd] =
                            cpu->regFile[cpu->instFields.rs1] - cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                    case SLL:  { // Shift left logical
                        TRACE_R((cpu), "sll");
                        cpu->regFile[cpu->instFields.rd] =
                            cpu->regFile[cpu->instFields.rs1] << cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                    case SLT:  { // Set if less than (signed)
                        TRACE_R((cpu), "slt");
                        cpu->regFile[cpu->instFields.rd] =
                            ((s32)cpu->regFile[cpu->instFields.rs1] < (s32)cpu->regFile[cpu->instFields.rs2]) ? 1 : 0;
                        break;
                    }
                    case SLTU: { // Set if less than (unsigned)
                        TRACE_R((cpu), "sltu");
                        cpu->regFile[cpu->instFields.rd] =
                            (cpu->regFile[cpu->instFields.rs1] < cpu->regFile[cpu->instFields.rs2]) ? 1 : 0;
                        break;
                    }
                    case XOR:  { // Bitwise xor
                        TRACE_R((cpu), "xor");
                        cpu->regFile[cpu->instFields.rd] =
                            cpu->regFile[cpu->instFields.rs1] ^ cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                    case SRL:  { // Shift right logical
                        TRACE_R((cpu), "srl");
                        cpu->regFile[cpu->instFields.rd] =
                            cpu->regFile[cpu->instFields.rs1] >> cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                    case SRA:  { // Shift right arithmetic
                        TRACE_R((cpu), "sra");
                        cpu->regFile[cpu->instFields.rd] =
                            (u32)((s32)cpu->regFile[cpu->instFields.rs1] >> cpu->regFile[cpu->instFields.rs2]);
                        break;
                    }
                    case OR:   { // Bitwise or
                        TRACE_R((cpu), "or");
                        cpu->regFile[cpu->instFields.rd] =
                            cpu->regFile[cpu->instFields.rs1] | cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                    case AND:  { // Bitwise and
                        TRACE_R((cpu), "and");
                        cpu->regFile[cpu->instFields.rd] =
                            cpu->regFile[cpu->instFields.rs1] & cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                }
                break;
            }
            case I: {
                // Decode
                cpu->instFields.rd     = GET_RD(cpu->IF);
                cpu->instFields.rs1    = GET_RS1(cpu->IF);
                cpu->instFields.funct3 = GET_FUNCT3(cpu->IF);
                cpu->immFields.imm11_0 = GET_IMM_11_0(cpu->IF);
                cpu->immFields.succ    = GET_SUCC(cpu->IF);
                cpu->immFields.pred    = GET_PRED(cpu->IF);
                cpu->immFields.fm      = GET_FM(cpu->IF);
                cpu->immFinal = (((s32)cpu->immFields.imm11_0 << 20) >> 20);
                cpu->ID = (cpu->instFields.funct3 << 7) | cpu->instFields.opcode;
                cpu->targetAddress = cpu->regFile[cpu->instFields.rs1] + cpu->immFinal;
                // Execute
                switch ((ItypeInstructions)cpu->ID) {
                    case JALR:  { // Jump and link register
                        TRACE_I((cpu), "jalr");
                        cpu->regFile[cpu->instFields.rd] = cpu->pc + 4;
                        cpu->pc = ((cpu->targetAddress) & 0xfffe) - 4;
                        break;
                    }
                    case LB:    { // Load byte (signed)
                        TRACE_L((cpu), "lb");
                        u32 loadByte = (u32)ACCESS_MEM_B(cpu->virtMem, cpu->targetAddress);
                        cpu->regFile[cpu->instFields.rd] = (u32)((s32)(loadByte << 24) >> 24);
                        break;
                    }
                    case LH:    { // Load halfword (signed)
                        TRACE_L((cpu), "lh");
                        u32 loadHalfword = (u32)ACCESS_MEM_H(cpu->virtMem, cpu->targetAddress);
                        cpu->regFile[cpu->instFields.rd] = (u32)((s32)(loadHalfword << 16) >> 16);
                        break;
                    }
                    case LW:    { // Load word
                        TRACE_L((cpu), "lw");
                        cpu->regFile[cpu->instFields.rd] =
                            ACCESS_MEM_W(cpu->virtMem, cpu->targetAddress);
                        break;
                    }
                    case LBU:   { // Load byte (unsigned)
                        TRACE_L((cpu), "lbu");
                        cpu->regFile[cpu->instFields.rd] =
                            (u32)ACCESS_MEM_B(cpu->virtMem, cpu->targetAddress);
                        break;
                    }
                    case LHU:   { // Load halfword (unsigned)
                        TRACE_L((cpu), "lhu");
                        cpu->regFile[cpu->instFields.rd] =
                            (u32)ACCESS_MEM_H(cpu->virtMem, cpu->targetAddress);
                        break;
                    }
                    case ADDI:  { // Add immediate
                        TRACE_I((cpu), "addi");
                        cpu->regFile[cpu->instFields.rd] = cpu->targetAddress;
                        break;
                    }
                    case SLTI:  { // Set if less than immediate (signed)
                        TRACE_I((cpu), "slti");
                        cpu->regFile[cpu->instFields.rd] =
                            ((s32)cpu->regFile[cpu->instFields.rs1] < cpu->immFinal) ? 1 : 0;
                        break;
                    }
                    case SLTIU: { // Set if less than immediate (unsigned)
                        TRACE_I((cpu), "sltiu");
                        cpu->regFile[cpu->instFields.rd] =
                            (cpu->regFile[cpu->instFields.rs1] < (u32)cpu->immFinal) ? 1 : 0;
                        break;
                    }
                    case XORI:  { // Bitwise exclusive or immediate
                        TRACE_I((cpu), "xori");
                        cpu->regFile[cpu->instFields.rd] = cpu->regFile[cpu->instFields.rs1] ^ cpu->immFinal;
                        break;
                    }
                    case ORI:   { // Bitwise or immediate
                        TRACE_I((cpu), "ori");
                        cpu->regFile[cpu->instFields.rd] = cpu->regFile[cpu->instFields.rs1] | cpu->immFinal;
                        break;
                    }
                    case ANDI:  { // Bitwise and immediate
                        TRACE_I((cpu), "andi");
                        cpu->regFile[cpu->instFields.rd] = cpu->regFile[cpu->instFields.rs1] & cpu->immFinal;
                        break;
                    }
                    case FENCE: { // FENCE - order device I/O and memory accesses
                        TRACE_FEN((cpu), "fence");
                        cpu->pfnEnvHandler(cpu);
                        break;
                    }
                    // Catch environment-type instructions
                    default: {
                        cpu->ID = (cpu->immFields.imm11_0 << 20) |
                            (cpu->instFields.funct3 << 7) | cpu->instFields.opcode;
                        switch ((ItypeInstructions)cpu->ID) {
                            case ECALL:  { // ECALL - request a syscall
                                TRACE_E((cpu), "ecall");
                                cpu->pfnEnvHandler(cpu);
                                break;
                            }
                            case EBREAK: { // EBREAK - halt processor execution, transfer control to debugger
                                TRACE_E((cpu), "ebreak");
                                cpu->pfnEnvHandler(cpu);
                                break;
                            }
                            default: { // Invalid instruction
                                printf("[rISA]: Error - (0x%08x) is an invalid instruction.\n", cpu->IF);
                                cpu->endTime = clock();
                                cleanupSimulator(cpu);
                                return EILSEQ;
                            }
                        }
                    }
                }
                break;
            }
            case S: {
                // Decode
                cpu->instFields.funct3 = GET_FUNCT3(cpu->IF);
                cpu->immFields.imm4_0  = GET_IMM_4_0(cpu->IF);
                cpu->instFields.rs1    = GET_RS1(cpu->IF);
                cpu->instFields.rs2    = GET_RS2(cpu->IF);
                cpu->immFields.imm11_5 = GET_IMM_11_5(cpu->IF);
                cpu->immPartial = cpu->immFields.imm4_0 | (cpu->immFields.imm11_5 << 5);
                cpu->immFinal = (((s32)cpu->immPartial << 20) >> 20);
                cpu->ID = (cpu->instFields.funct3 << 7) | cpu->instFields.opcode;
                cpu->targetAddress = cpu->regFile[cpu->instFields.rs1] + cpu->immFinal;
                // Execute
                switch ((StypeInstructions)cpu->ID) {
                    case SB: { // Store byte
                        TRACE_S((cpu), "sb");
                        ACCESS_MEM_B(cpu->virtMem, cpu->targetAddress) =
                            (u8)cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                    case SH: { // Store halfword
                        TRACE_S((cpu), "sh");
                        ACCESS_MEM_H(cpu->virtMem, cpu->targetAddress) =
                            (u16)cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                    case SW: { // Store word
                        TRACE_S((cpu), "sw");
                        ACCESS_MEM_W(cpu->virtMem, cpu->targetAddress) =
                            cpu->regFile[cpu->instFields.rs2];
                        break;
                    }
                }
                cpu->pfnMmioHandler(cpu);
                break;
            }
            case B: {
                // Decode
                cpu->instFields.rs1    = GET_RS1(cpu->IF);
                cpu->instFields.rs2    = GET_RS2(cpu->IF);
                cpu->instFields.funct3 = GET_FUNCT3(cpu->IF);
                cpu->immFields.imm11   = GET_IMM_11_B(cpu->IF);
                cpu->immFields.imm4_1  = GET_IMM_4_1(cpu->IF);
                cpu->immFields.imm10_5 = GET_IMM_10_5(cpu->IF);
                cpu->immFields.imm12   = GET_IMM_12(cpu->IF);
                cpu->immPartial = cpu->immFields.imm4_1 | (cpu->immFields.imm10_5 << 4) |
                    (cpu->immFields.imm11 << 10) | (cpu->immFields.imm12 << 11);
                cpu->targetAddress = (s32)(cpu->immPartial << 20) >> 19;
                cpu->ID = (cpu->instFields.funct3 << 7) | cpu->instFields.opcode;
                // Execute
                switch ((BtypeInstructions)cpu->ID) {
                    case BEQ:  { // Branch if Equal
                        TRACE_B((cpu), "beq");
                        if ((s32)cpu->regFile[cpu->instFields.rs1] == (s32)cpu->regFile[cpu->instFields.rs2]) {
                            cpu->pc += cpu->targetAddress - 4;
                        }
                        break;
                    }
                    case BNE:  { // Branch if Not Equal
                        TRACE_B((cpu), "bne");
                        if ((s32)cpu->regFile[cpu->instFields.rs1] != (s32)cpu->regFile[cpu->instFields.rs2]) {
                            cpu->pc += cpu->targetAddress - 4;
                        }
                        break;
                    }
                    case BLT:  { // Branch if Less Than
                        TRACE_B((cpu), "blt");
                        if ((s32)cpu->regFile[cpu->instFields.rs1] < (s32)cpu->regFile[cpu->instFields.rs2]) {
                            cpu->pc += cpu->targetAddress - 4;
                        }
                        break;
                    }
                    case BGE:  { // Branch if Greater Than or Equal
                        TRACE_B((cpu), "bge");
                        if ((s32)cpu->regFile[cpu->instFields.rs1] >= (s32)cpu->regFile[cpu->instFields.rs2]) {
                            cpu->pc += cpu->targetAddress - 4;
                        }
                        break;
                    }
                    case BLTU: { // Branch if Less Than (unsigned)
                        TRACE_B((cpu), "bltu");
                        if (cpu->regFile[cpu->instFields.rs1] < cpu->regFile[cpu->instFields.rs2]) {
                            cpu->pc += cpu->targetAddress - 4;
                        }
                        break;
                    }
                    case BGEU: { // Branch if Greater Than or Equal (unsigned)
                        TRACE_B((cpu), "bgeu");
                        if (cpu->regFile[cpu->instFields.rs1] >= cpu->regFile[cpu->instFields.rs2]) {
                            cpu->pc += cpu->targetAddress - 4;
                        }
                        break;
                    }
                }
                break;
            }
            case U: {
                // Decode
                cpu->instFields.rd      = GET_RD(cpu->IF);
                cpu->immFields.imm31_12 = GET_IMM_31_12(cpu->IF);
                cpu->immFinal = cpu->immFields.imm31_12 << 12;
                // Execute
                switch ((UtypeInstructions)cpu->instFields.opcode) {
                    case LUI:   { // Load Upper Immediate
                        TRACE_U((cpu), "lui");
                        cpu->regFile[cpu->instFields.rd] = cpu->immFinal;
                        break;
                    }
                    case AUIPC: { // Add Upper Immediate to cpu->pc
                        TRACE_U((cpu), "auipc");
                        cpu->regFile[cpu->instFields.rd] = cpu->pc + cpu->immFinal;
                        break;
                    }
                }
                break;
            }
            case J: { // Jump and link
                // Decode
                cpu->instFields.rd      = GET_RD(cpu->IF);
                cpu->immFields.imm19_12 = GET_IMM_19_12(cpu->IF);
                cpu->immFields.imm11    = GET_IMM_11_J(cpu->IF);
                cpu->immFields.imm10_1  = GET_IMM_10_1(cpu->IF);
                cpu->immFields.imm20    = GET_IMM_20(cpu->IF);
                cpu->immPartial = cpu->immFields.imm10_1 | (cpu->immFields.imm11 << 10) |
                    (cpu->immFields.imm19_12 << 11) | (cpu->immFields.imm20 << 19);
                cpu->targetAddress = (s32)(cpu->immPartial << 12) >> 11;
                TRACE_J((cpu), "jal");
                // Execute
                cpu->regFile[cpu->instFields.rd] = cpu->pc + 4;
                cpu->pc += cpu->targetAddress - 4;
                break;
            }
            default: { // Invalid instruction
                printf("[rISA]: Error - (0x%08x) is an invalid instruction.\n", cpu->IF);
                cpu->endTime = clock();
                cleanupSimulator(cpu);
                return EILSEQ;
            }
        }
        cpu->cycleCounter++;
        cpu->pc += 4;
        if (g_sigIntDet) { // Normal exit/cleanup
            cpu->endTime = clock();
            cleanupSimulator(cpu);
            return 0;
        }
        if (cpu->pc > cpu->virtMemSize) {
            printf("[rISA]: Error. Program counter is out of range.\n");
            cpu->endTime = clock();
            cleanupSimulator(cpu);
            return EFAULT;
        }
        if ((cpu->cycleCounter % cpu->intPeriodVal) == 0) {
            cpu->pfnIntHandler(cpu);
        }
        cpu->regFile[0] = 0;
    }
}

const char *g_regfileAliasLookup[] = {
    "zero",
    "ra",
    "sp",
    "gp", "tp",
    "t0", "t1", "t2",
    "s0",
    "s1",
    "a0", "a1",
    "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6"
};

const InstFormats g_opcodeToFormat[128] = {
    /* 0b0000000 */ Undefined,
    /* 0b0000001 */ Undefined,
    /* 0b0000010 */ Undefined,
    /* 0b0000011 */ I,
    /* 0b0000100 */ Undefined,
    /* 0b0000101 */ Undefined,
    /* 0b0000110 */ Undefined,
    /* 0b0000111 */ Undefined,
    /* 0b0001000 */ Undefined,
    /* 0b0001001 */ Undefined,
    /* 0b0001010 */ Undefined,
    /* 0b0001011 */ Undefined,
    /* 0b0001100 */ Undefined,
    /* 0b0001101 */ Undefined,
    /* 0b0001110 */ Undefined,
    /* 0b0001111 */ I,
    /* 0b0010000 */ Undefined,
    /* 0b0010001 */ Undefined,
    /* 0b0010010 */ Undefined,
    /* 0b0010011 */ I,
    /* 0b0010100 */ Undefined,
    /* 0b0010101 */ Undefined,
    /* 0b0010110 */ Undefined,
    /* 0b0010111 */ U,
    /* 0b0011000 */ Undefined,
    /* 0b0011001 */ Undefined,
    /* 0b0011010 */ Undefined,
    /* 0b0011011 */ Undefined,
    /* 0b0011100 */ Undefined,
    /* 0b0011101 */ Undefined,
    /* 0b0011110 */ Undefined,
    /* 0b0011111 */ Undefined,
    /* 0b0100000 */ Undefined,
    /* 0b0100001 */ Undefined,
    /* 0b0100010 */ Undefined,
    /* 0b0100011 */ S,
    /* 0b0100100 */ Undefined,
    /* 0b0100101 */ Undefined,
    /* 0b0100110 */ Undefined,
    /* 0b0100111 */ Undefined,
    /* 0b0101000 */ Undefined,
    /* 0b0101001 */ Undefined,
    /* 0b0101010 */ Undefined,
    /* 0b0101011 */ Undefined,
    /* 0b0101100 */ Undefined,
    /* 0b0101101 */ Undefined,
    /* 0b0101110 */ Undefined,
    /* 0b0101111 */ Undefined,
    /* 0b0110000 */ Undefined,
    /* 0b0110001 */ Undefined,
    /* 0b0110010 */ Undefined,
    /* 0b0110011 */ R,
    /* 0b0110100 */ Undefined,
    /* 0b0110101 */ Undefined,
    /* 0b0110110 */ Undefined,
    /* 0b0110111 */ U,
    /* 0b0111000 */ Undefined,
    /* 0b0111001 */ Undefined,
    /* 0b0111010 */ Undefined,
    /* 0b0111011 */ Undefined,
    /* 0b0111100 */ Undefined,
    /* 0b0111101 */ Undefined,
    /* 0b0111110 */ Undefined,
    /* 0b0111111 */ Undefined,
    /* 0b1000000 */ Undefined,
    /* 0b1000001 */ Undefined,
    /* 0b1000010 */ Undefined,
    /* 0b1000011 */ Undefined,
    /* 0b1000100 */ Undefined,
    /* 0b1000101 */ Undefined,
    /* 0b1000110 */ Undefined,
    /* 0b1000111 */ Undefined,
    /* 0b1001000 */ Undefined,
    /* 0b1001001 */ Undefined,
    /* 0b1001010 */ Undefined,
    /* 0b1001011 */ Undefined,
    /* 0b1001100 */ Undefined,
    /* 0b1001101 */ Undefined,
    /* 0b1001110 */ Undefined,
    /* 0b1001111 */ Undefined,
    /* 0b1010000 */ Undefined,
    /* 0b1010001 */ Undefined,
    /* 0b1010010 */ Undefined,
    /* 0b1010011 */ Undefined,
    /* 0b1010100 */ Undefined,
    /* 0b1010101 */ Undefined,
    /* 0b1010110 */ Undefined,
    /* 0b1010111 */ Undefined,
    /* 0b1011000 */ Undefined,
    /* 0b1011001 */ Undefined,
    /* 0b1011010 */ Undefined,
    /* 0b1011011 */ Undefined,
    /* 0b1011100 */ Undefined,
    /* 0b1011101 */ Undefined,
    /* 0b1011110 */ Undefined,
    /* 0b1011111 */ Undefined,
    /* 0b1100000 */ Undefined,
    /* 0b1100001 */ Undefined,
    /* 0b1100010 */ Undefined,
    /* 0b1100011 */ B,
    /* 0b1100100 */ Undefined,
    /* 0b1100101 */ Undefined,
    /* 0b1100110 */ Undefined,
    /* 0b1100111 */ J,
    /* 0b1101000 */ Undefined,
    /* 0b1101001 */ Undefined,
    /* 0b1101010 */ Undefined,
    /* 0b1101011 */ Undefined,
    /* 0b1101100 */ Undefined,
    /* 0b1101101 */ Undefined,
    /* 0b1101110 */ Undefined,
    /* 0b1101111 */ J,
    /* 0b1110000 */ Undefined,
    /* 0b1110001 */ Undefined,
    /* 0b1110010 */ Undefined,
    /* 0b1110011 */ I,
    /* 0b1110100 */ Undefined,
    /* 0b1110101 */ Undefined,
    /* 0b1110110 */ Undefined,
    /* 0b1110111 */ Undefined,
    /* 0b1111000 */ Undefined,
    /* 0b1111001 */ Undefined,
    /* 0b1111010 */ Undefined,
    /* 0b1111011 */ Undefined,
    /* 0b1111100 */ Undefined,
    /* 0b1111101 */ Undefined,
    /* 0b1111110 */ Undefined,
    /* 0b1111111 */ Undefined
};