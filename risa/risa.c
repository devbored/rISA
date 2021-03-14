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

void stubMmioHandler(rv32iHart *cpu)  { return; }
void stubIntHandler(rv32iHart *cpu)   { return; }
void stubEnvHandler(rv32iHart *cpu)   { return; }
void stubInitHandler(rv32iHart *cpu)  { return; }
void stubExitHandler(rv32iHart *cpu)  { return; }

void printHelp(void) {
    printf(
        "[rISA]:    Usage: risa [OPTIONS] <program_binary>\n"
        "           Example: risa -m 1024 my_riscv_program.hex"
        "\n\n"
        "           OPTIONS:\n"
        "               -m <int>  : Virtual memory/IO size (in bytes).\n"
        "               -l <file> : Dynamic library file to user-defined handler functions.\n"
        "               -t        : Enable trace printing to stderr.\n"
        "               -i <int>  : Simulator interrupt-check timeout value [DEFAULT=500].\n"
        "               -h        : Print help and exit.\n"
        "\n"
    );
}

void cleanupSimulator(rv32iHart *cpu, int err) {
    if (cpu->pfnExitHandler != NULL) { cpu->pfnExitHandler(cpu);    }
    if (cpu->virtMem != NULL)        { free(cpu->virtMem);          }
    if (cpu->mmioData != NULL)       { free(cpu->mmioData);         }
    if (cpu->intData != NULL)        { free(cpu->intData);          }
    if (cpu->envData != NULL)        { free(cpu->envData);          }
    if (cpu->handlerLib != NULL)     { CLOSE_LIB(cpu->handlerLib);  }
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

SimulatorOptions isOption(const char *arg) {
    if      (strcmp(arg, "-m") == 0)    { return OPT_VIRT_MEM_SIZE; }
    else if (strcmp(arg, "-l") == 0)    { return OPT_HANDLER_LIB;   }
    else if (strcmp(arg, "-h") == 0)    { return OPT_HELP;          }
    else if (strcmp(arg, "-t") == 0)    { return OPT_TRACING;       }
    else if (strcmp(arg, "-i") == 0)    { return OPT_INTERRUPT;     }
    else                                { return OPT_UNKNOWN;       }
}

void processOptions(int argc, char** argv, rv32iHart *cpu) {
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
            case OPT_TRACING: // Enable trace printing ("-t")
                cpu->opts.o_tracePrintEnable = 1;
                break;
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

void loadProgram(int argc, char **argv, rv32iHart *cpu) {
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

void setupSimulator(int argc, char **argv, rv32iHart *cpu) {
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
}

const InstFormats OpcodeToFormat[128] = {
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