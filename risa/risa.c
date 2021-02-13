#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "risa.h"

static void stubMmioHandler(rv32iHart *cpu)  { return; }
static void stubIntHandler(rv32iHart *cpu)   { return; }
static void stubEnvHandler(rv32iHart *cpu)   { return; }
static void stubInitHandler(rv32iHart *cpu)  { return; }
static void stubExitHandler(rv32iHart *cpu)  { return; }

static volatile int g_sigIntDet = 0;
static SIGINT_RET_TYPE sigintHandler(SIGINT_PARAM sig) { 
    g_sigIntDet = 1;
    SIGINT_RET;
}

static void printHelp(void) {
    printf(
        "[rISA]:    Usage: risa [OPTIONS] <program_binary>\n"
        "           Example: risa -m 1024 my_riscv_program.hex"
        "\n\n"
        "           OPTIONS:\n"
        "               -m <int>  : Virtual memory/IO size (in bytes).\n"
        "               -l <file> : Dynamic library file to user-defined handler functions.\n"
        "               -t <int>  : Simulator cycle timeout value [DEFAULT=INT32_MAX].\n"
        "               -i <int>  : Simulator interrupt-check timeout value [DEFAULT=500].\n"
        "               -h        : Print help and exit.\n"
        "               -p        : Enable debug printing.\n"
        "\n"
    );
}

static void cleanupSimulator(rv32iHart *cpu, int err) {
    if (cpu->pfnExitHandler != NULL) { cpu->pfnExitHandler(cpu); }
    if (cpu->virtMem != NULL)        { free(cpu->virtMem);  }
    if (cpu->mmioData != NULL)       { free(cpu->mmioData); }
    if (cpu->intData != NULL)        { free(cpu->intData);  }
    if (cpu->envData != NULL)        { free(cpu->envData);  }
    if (cpu->handlerLib != NULL)     { CLOSE_LIB(cpu->handlerLib); }
    cpu->timeDelta = ((double)(cpu->endTime - cpu->startTime)) / CLOCKS_PER_SEC;
    printf(
        "[rISA]: Simulator stopping.\n"
        "        Simulation time elapsed: (%f seconds).\n",
        cpu->timeDelta
    );
    switch (err) {
        case ENOMEM:    exit(ENOMEM);
        case EINVAL:    exit(EINVAL);
        case EFAULT:    exit(EFAULT);
        case EILSEQ:    exit(EILSEQ);
        case ECANCELED: exit(ECANCELED);
        case EIO:       exit(EIO);
        default:        exit(0);
    }
}

static SimulatorOptions isOption(const char *arg) {
    if      (strcmp(arg, "-m") == 0)    { return OPT_VIRT_MEM_SIZE; }
    else if (strcmp(arg, "-l") == 0)    { return OPT_HANDLER_LIB; }
    else if (strcmp(arg, "-h") == 0)    { return OPT_HELP; }
    else if (strcmp(arg, "-p") == 0)    { return OPT_DEBUG; }
    else if (strcmp(arg, "-t") == 0)    { return OPT_TIMEOUT; }
    else if (strcmp(arg, "-i") == 0)    { return OPT_INTERRUPT; }
    else                                { return OPT_UNKNOWN; }
}

static void processOptions(int argc, char** argv, rv32iHart *cpu) {
    for (int i=1; i<argc; ++i) {
        if (((i+1) != argc) && (isOption(argv[i]) & VALUE_OPTS) && (isOption(argv[i+1]) & VALUE_OPTS)) {
            printf("[rISA]: ERROR - Invalid formatting of options.\n");
            printHelp();
            cleanupSimulator(cpu, EINVAL);
        }
        if (((i+1) == argc) && (isOption(argv[i]) & VALUE_OPTS)) {
            printf("[rISA]: ERROR - Invalid formatting of options.\n");
            printHelp();
            cleanupSimulator(cpu, EINVAL);
        }
        SimulatorOptions opt = isOption(argv[i]);
        switch (opt) {
            case OPT_VIRT_MEM_SIZE: { // Attempt to allocate virtual memory ("-m")
                if (cpu->opts.o_virtMemSize) {
                    printf("[rISA]: ERROR - Ambiguous multiple '-m' options defined - specify only one.\n");
                    printHelp();
                    cleanupSimulator(cpu, EINVAL);
                }
                int size = atoi(argv[i+1]);
                if (size == 0) {
                    printf("[rISA]: INFO - Value <%s> is not a valid integer. Defauting to <%d>\n", 
                        argv[i+1], DEFAULT_VIRT_MEM_SIZE);
                    cpu->virtMem = malloc(DEFAULT_VIRT_MEM_SIZE);
                }
                else { 
                    cpu->virtMem = malloc(size);
                }
                cpu->virtMemRange = (size == 0) ? DEFAULT_VIRT_MEM_SIZE : size;
                if (cpu->virtMem == NULL) {
                    printf("[rISA]: ERROR - Could not allocate virtual memory.\n");
                    cleanupSimulator(cpu, ENOMEM);
                }
                cpu->opts.o_virtMemSize = 1;
                break;
            }
            case OPT_HANDLER_LIB: { // Attempt to load user handlers ("-l")
                if (cpu->opts.o_definedHandles) {
                    printf("[rISA]: ERROR - Ambiguous multiple '-l' options defined (specify only one).\n");
                    printHelp();
                    cleanupSimulator(cpu, EINVAL);
                }
                cpu->handlerLib = LOAD_LIB(argv[i+1]);
                if (cpu->handlerLib == NULL) {
                    printf(
                        "[rISA]: INFO - Could not load dynamic library <%s>.\n"
                        "        Using default stub handlers instead.\n", argv[i+1]
                    );
                }
                pfnMmioHandler mmioHandle = (pfnMmioHandler)LOAD_SYM(cpu->handlerLib, "risaMmioHandler");
                if (mmioHandle != NULL) { cpu->pfnMmioHandler = mmioHandle; }
                else { printf("[rISA]: INFO - Could not load function risaMmioHandler. Defaulting to stub.\n"); }
                pfnIntHandler intHandle = (pfnIntHandler)LOAD_SYM(cpu->handlerLib, "risaIntHandler");
                if (intHandle != NULL) { cpu->pfnIntHandler = intHandle; }
                else { printf("[rISA]: INFO - Could not load function risaIntHandler. Defaulting to stub.\n"); }
                pfnEnvHandler envHandle = (pfnEnvHandler)LOAD_SYM(cpu->handlerLib, "risaEnvHandler");
                if (envHandle != NULL) { cpu->pfnEnvHandler = envHandle; }
                else { printf("[rISA]: INFO - Could not load function risaEnvHandler. Defaulting to stub.\n"); }
                pfnInitHandler initHandle = (pfnInitHandler)LOAD_SYM(cpu->handlerLib, "risaInitHandler");
                if (initHandle != NULL) { cpu->pfnInitHandler = initHandle; }
                else { printf("[rISA]: INFO - Could not load function risaInitHandler. Defaulting to stub.\n"); }
                pfnExitHandler exitHandle = (pfnExitHandler)LOAD_SYM(cpu->handlerLib, "risaExitHandler");
                if (exitHandle != NULL) { cpu->pfnExitHandler = exitHandle; }
                else { printf("[rISA]: INFO - Could not load function risaExitHandler. Defaulting to stub.\n"); }
                cpu->opts.o_definedHandles = 1;
                break;
            }
            case OPT_HELP: // Print help menu ("-h")
                printHelp();
                cleanupSimulator(cpu, 0);
            case OPT_DEBUG: // Enable debug print ("-p")
                cpu->opts.o_debugPrintEnable = 1;
                break;
            case OPT_TIMEOUT: { // Configure a simulator timeout value ("-t")
                if (cpu->opts.o_timeout) {
                    printf("[rISA]: ERROR - Ambiguous multiple '-t' options defined (specify only one).\n");
                    printHelp();
                    cleanupSimulator(cpu, EINVAL);
                }
                long timeout = atol(argv[i+1]);
                if (timeout == 0) {
                    printf("[rISA]: INFO - Value <%s> is not a valid integer. Defauting to <%d>\n", 
                        argv[i+1], DEFAULT_VIRT_MEM_SIZE);
                    cpu->timeoutVal = DEFAULT_VIRT_MEM_SIZE;
                }
                else { 
                    cpu->timeoutVal = timeout;
                }
                cpu->timeoutVal = (timeout == 0) ? INT32_MAX : timeout;
                cpu->opts.o_timeout = 1;
                break;
            }
            case OPT_INTERRUPT: { // Configure a simulator interrupt period value ("-i")
                if (cpu->opts.o_intPeriod) {
                    printf("[rISA]: ERROR - Ambiguous multiple '-i' options defined (specify only one).\n");
                    printHelp();
                    cleanupSimulator(cpu, EINVAL);
                }
                int timeout = atoi(argv[i+1]);
                if (timeout == 0) {
                    printf("[rISA]: INFO - Value <%s> is not a valid integer. Defauting to <%d>\n", 
                        argv[i+1], DEFAULT_VIRT_MEM_SIZE);
                    cpu->intPeriodVal = DEFAULT_VIRT_MEM_SIZE;
                }
                else { 
                    cpu->intPeriodVal = timeout;
                }
                cpu->intPeriodVal = (timeout == 0) ? DEFAULT_INT_PERIOD : timeout;
                cpu->opts.o_intPeriod = 1;
                break;
            }
            default: // Unknown value if prior arg was not a valid OPTION <value>
                if ((i != argc-1) && !(isOption(argv[i-1]) & VALUE_OPTS)) {
                    printf("[rISA]: ERROR - Unknown option <%s>.\n", argv[i]);
                    printHelp();
                    cleanupSimulator(cpu, EINVAL);
                }
            break;
        }
    }
}

static void loadProgram(int argc, char **argv, rv32iHart *cpu) {
    if ((isOption(argv[argc-1]) != OPT_UNKNOWN) || (isOption(argv[argc-2]) == VALUE_OPTS)) {
        printf("[rISA]: ERROR - No program specified.\n");
        printHelp();
        cleanupSimulator(cpu, EINVAL);
    }
    FILE* binFile;
    OPEN_FILE(binFile, argv[argc-1], "rb");
    if (binFile == NULL) {
        printf("[rISA]: ERROR - Could not open file <%s>.\n", argv[argc-1]);
        printHelp();
        cleanupSimulator(cpu, EIO);
    }
    for (int i=0; !feof(binFile) != 0; ++i) {
        if (i >= (cpu->virtMemRange / sizeof(u32))) {
            printf("[rISA]: ERROR - Could not fit <%s> in simulator's virtual memory.\n", argv[argc-1]);
            fclose(binFile);
            cleanupSimulator(cpu, ENOMEM);
        }
        fread(cpu->virtMem+i, sizeof(u32), 1, binFile);
    }
    fclose(binFile);
}

static void setupSimulator(int argc, char **argv, rv32iHart *cpu) {
    if (argc == 1) {
        printf("[rISA]: ERROR - No program specified.\n");
        printHelp();
        cleanupSimulator(cpu, EINVAL);
    }
    processOptions(argc, argv, cpu);
    if (cpu->pfnMmioHandler == NULL)  { cpu->pfnMmioHandler = stubMmioHandler;    }
    if (cpu->pfnIntHandler  == NULL)  { cpu->pfnIntHandler  = stubIntHandler;     }
    if (cpu->pfnEnvHandler  == NULL)  { cpu->pfnEnvHandler  = stubEnvHandler;     }
    if (cpu->pfnInitHandler == NULL)  { cpu->pfnInitHandler = stubInitHandler;    }
    if (cpu->pfnExitHandler == NULL)  { cpu->pfnExitHandler = stubExitHandler;    }
    if (cpu->intPeriodVal   == 0)     { cpu->intPeriodVal   = DEFAULT_INT_PERIOD; }
    if (cpu->timeoutVal     == 0)     { cpu->timeoutVal     = INT32_MAX;          }
    if (cpu->virtMem == NULL) {
        cpu->virtMem = malloc(DEFAULT_VIRT_MEM_SIZE);
        if (cpu->virtMem == NULL) {
            printf("[rISA]: ERROR - Could not allocate virtual memory.\n");
            cleanupSimulator(cpu, ENOMEM);
        }
        cpu->virtMemRange = DEFAULT_VIRT_MEM_SIZE;
    }
    loadProgram(argc, argv, cpu);
    cpu->pfnInitHandler(cpu);
    SIGINT_REGISTER(cpu, sigintHandler);
}

int main(int argc, char** argv) {
    rv32iHart cpu = {0};
    setupSimulator(argc, argv, &cpu);
    printf("[rISA]: Running simulator...\n\n");
    cpu.startTime = clock();
    for (;;) {
        // Fetch
        cpu.IF = ACCESS_MEM_W(cpu.virtMem, cpu.pc);
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
                        DEBUG_PRINT((&cpu), "Current instruction: slli x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SRLI: { // Shift right logical by immediate (i.e. rs2 is shamt)
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] >> cpu.instFields.rs2;
                        DEBUG_PRINT((&cpu), "Current instruction: srli x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SRAI: { // Shift right arithmetic by immediate (i.e. rs2 is shamt)
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)((s32)cpu.regFile[cpu.instFields.rs1] >> cpu.instFields.rs2);
                        DEBUG_PRINT((&cpu), "Current instruction: srai x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case ADD:  { // Addition
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] + cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: add x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SUB:  { // Subtraction
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] - cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: sub x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SLL:  { // Shift left logical
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] << cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: sll x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SLT:  { // Set if less than (signed)
                        cpu.regFile[cpu.instFields.rd] = 
                            ((s32)cpu.regFile[cpu.instFields.rs1] < (s32)cpu.regFile[cpu.instFields.rs2]) ? 1 : 0;
                        DEBUG_PRINT((&cpu), "Current instruction: slt x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SLTU: { // Set if less than (unsigned)
                        cpu.regFile[cpu.instFields.rd] = 
                            (cpu.regFile[cpu.instFields.rs1] < cpu.regFile[cpu.instFields.rs2]) ? 1 : 0;
                        DEBUG_PRINT((&cpu), "Current instruction: sltu x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case XOR:  { // Bitwise xor
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] ^ cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: xor x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SRL:  { // Shift right logical
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] >> cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: srl x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case SRA:  { // Shift right arithmetic
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)((s32)cpu.regFile[cpu.instFields.rs1] >> cpu.regFile[cpu.instFields.rs2]);
                        DEBUG_PRINT((&cpu), "Current instruction: sra x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case OR:   { // Bitwise or
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] | cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: or x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.instFields.rs2);
                        break;
                    }
                    case AND:  { // Bitwise and
                        cpu.regFile[cpu.instFields.rd] = 
                            cpu.regFile[cpu.instFields.rs1] & cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: and x%d, x%d, %d\n",
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
                cpu.targetAddress = cpu.regFile[cpu.instFields.rs1] + cpu.immFinal;
                // Execute
                switch ((ItypeInstructions)cpu.ID) {
                    case JALR:  { // Jump and link register
                        cpu.regFile[cpu.instFields.rd] = cpu.pc + 4;
                        cpu.pc = ((cpu.targetAddress) & 0xfffe) - 4;
                        DEBUG_PRINT((&cpu), "Current instruction: jalr x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case LB:    { // Load byte (signed)
                        u32 loadByte = (u32)ACCESS_MEM_B(cpu.virtMem, cpu.targetAddress);
                        cpu.regFile[cpu.instFields.rd] = (u32)((s32)(loadByte << 24) >> 24);
                        DEBUG_PRINT((&cpu), "Current instruction: lb x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case LH:    { // Load halfword (signed)
                        u32 loadHalfword = (u32)ACCESS_MEM_H(cpu.virtMem, cpu.targetAddress);
                        cpu.regFile[cpu.instFields.rd] = (u32)((s32)(loadHalfword << 16) >> 16);
                        DEBUG_PRINT((&cpu), "Current instruction: lh x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case LW:    { // Load word
                        cpu.regFile[cpu.instFields.rd] = 
                            ACCESS_MEM_W(cpu.virtMem, cpu.targetAddress);
                        DEBUG_PRINT((&cpu), "Current instruction: lw x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case LBU:   { // Load byte (unsigned)
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)ACCESS_MEM_B(cpu.virtMem, cpu.targetAddress);
                        DEBUG_PRINT((&cpu), "Current instruction: lbu x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case LHU:   { // Load halfword (unsigned)
                        cpu.regFile[cpu.instFields.rd] = 
                            (u32)ACCESS_MEM_H(cpu.virtMem, cpu.targetAddress);
                        DEBUG_PRINT((&cpu), "Current instruction: lhu x%d, %d(x%d)\n",
                            cpu.instFields.rd, cpu.immFields.imm11_0, cpu.instFields.rs1);
                        break;
                    }
                    case ADDI:  { // Add immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.targetAddress;
                        DEBUG_PRINT((&cpu), "Current instruction: addi x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case SLTI:  { // Set if less than immediate (signed)
                        cpu.regFile[cpu.instFields.rd] = 
                            ((s32)cpu.regFile[cpu.instFields.rs1] < cpu.immFinal) ? 1 : 0;
                        DEBUG_PRINT((&cpu), "Current instruction: slti x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case SLTIU: { // Set if less than immediate (unsigned)
                        cpu.regFile[cpu.instFields.rd] = 
                            (cpu.regFile[cpu.instFields.rs1] < (u32)cpu.immFinal) ? 1 : 0;
                        DEBUG_PRINT((&cpu), "Current instruction: sltiu x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case XORI:  { // Bitwise exclusive or immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] ^ cpu.immFinal;
                        DEBUG_PRINT((&cpu), "Current instruction: xori x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case ORI:   { // Bitwise or immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] | cpu.immFinal;
                        DEBUG_PRINT((&cpu), "Current instruction: ori x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case ANDI:  { // Bitwise and immediate
                        cpu.regFile[cpu.instFields.rd] = cpu.regFile[cpu.instFields.rs1] & cpu.immFinal;
                        DEBUG_PRINT((&cpu), "Current instruction: andi x%d, x%d, %d\n",
                            cpu.instFields.rd, cpu.instFields.rs1, cpu.immFields.imm11_0);
                        break;
                    }
                    case FENCE: { // FENCE - order device I/O and memory accesses
                        DEBUG_PRINT((&cpu), "Current instruction: fence %d, %d\n",
                            cpu.immFields.pred, cpu.immFields.succ);
                        cpu.pfnEnvHandler(&cpu);
                        break;
                    }
                    // Catch environment-type instructions
                    default: {
                        cpu.ID = (cpu.immFields.imm11_0 << 20) | (cpu.instFields.funct3 << 7) | cpu.instFields.opcode;
                        switch ((ItypeInstructions)cpu.ID) {
                            case ECALL: { // ECALL - request a syscall
                                DEBUG_PRINT((&cpu), "Current instruction: ecall\n");
                                break;
                            }
                            case EBREAK: { // EBREAK - halt processor execution, transfer control to debugger
                                DEBUG_PRINT((&cpu), "Current instruction: ebreak\n");
                                break;
                            }
                        }
                        cpu.pfnEnvHandler(&cpu);
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
                cpu.targetAddress = cpu.regFile[cpu.instFields.rs1] + cpu.immFinal;
                // Execute
                switch ((StypeInstructions)cpu.ID) {
                    case SB: { // Store byte
                        ACCESS_MEM_B(cpu.virtMem, cpu.targetAddress) =
                            (u8)cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: sb x%d, %d(x%d)\n",
                            cpu.instFields.rs2, cpu.immPartial, cpu.instFields.rs1);
                        break;
                    }
                    case SH: { // Store halfword
                        ACCESS_MEM_H(cpu.virtMem, cpu.targetAddress) =
                            (u16)cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: sh x%d, %d(x%d)\n",
                            cpu.instFields.rs2, cpu.immPartial, cpu.instFields.rs1);
                        break;
                    }
                    case SW: { // Store word
                        ACCESS_MEM_W(cpu.virtMem, cpu.targetAddress) =
                            cpu.regFile[cpu.instFields.rs2];
                        DEBUG_PRINT((&cpu), "Current instruction: sw x%d, %d(x%d)\n",
                            cpu.instFields.rs2, cpu.immPartial, cpu.instFields.rs1);
                        break;
                    }
                }
                cpu.pfnMmioHandler(&cpu);
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
                        DEBUG_PRINT((&cpu), "Current instruction: beq x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BNE:  { // Branch if Not Equal
                        if ((s32)cpu.regFile[cpu.instFields.rs1] != (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT((&cpu), "Current instruction: bne x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BLT:  { // Branch if Less Than
                        if ((s32)cpu.regFile[cpu.instFields.rs1] < (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT((&cpu), "Current instruction: blt x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BGE:  { // Branch if Greater Than or Equal
                        if ((s32)cpu.regFile[cpu.instFields.rs1] >= (s32)cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT((&cpu), "Current instruction: bge x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BLTU: { // Branch if Less Than (unsigned)
                        if (cpu.regFile[cpu.instFields.rs1] < cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT((&cpu), "Current instruction: bltu x%d, x%d, %d\n",
                            cpu.instFields.rs1, cpu.instFields.rs2, cpu.immFinal);
                        break;
                    }
                    case BGEU: { // Branch if Greater Than or Equal (unsigned)
                        if (cpu.regFile[cpu.instFields.rs1] >= cpu.regFile[cpu.instFields.rs2]) {
                            cpu.pc += cpu.immFinal - 4;
                        }
                        DEBUG_PRINT((&cpu), "Current instruction: bgeu x%d, x%d, %d\n",
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
                        DEBUG_PRINT((&cpu), "Current instruction: lui x%d, %d\n",
                            cpu.instFields.rd, cpu.immFinal);
                        break;
                    }
                    case AUIPC: { // Add Upper Immediate to cpu.pc
                        cpu.regFile[cpu.instFields.rd] = cpu.pc + cpu.immFinal;
                        DEBUG_PRINT((&cpu), "Current instruction: auipc x%d, %d\n",
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
                        DEBUG_PRINT((&cpu), "Current instruction: jal x%d, %d\n",
                            cpu.instFields.rd, cpu.immFinal);
                        break;
                    }
                }
                break;
            }
            default: { // Invalid instruction
                DEBUG_PRINT((&cpu), "Error. (0x%08x) is an invalid instruction.\n", cpu.IF);
                cpu.endTime = clock();
                cleanupSimulator(&cpu, EILSEQ);
            }
        }
        // Update cpu.pc and counter, check for interrupts, and reset register x0 back to zero
        cpu.cycleCounter++;
        DEBUG_PRINT((&cpu), "<%d> cycle(s)\n\n", cpu.cycleCounter);
        if (cpu.cycleCounter == cpu.timeoutVal || g_sigIntDet) { // Normal exit/cleanup
            cpu.endTime = clock();
            cleanupSimulator(&cpu, 0);
        }
        cpu.pc += 4;
        if (cpu.pc > cpu.virtMemRange) {
            DEBUG_PRINT((&cpu), "Error. Program counter is out of range.\n");
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
