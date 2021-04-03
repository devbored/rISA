typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;

#ifdef _WIN32 // --- windows
#define LOAD_LIB(libpath)               LoadLibrary(libpath)
#define CLOSE_LIB(handle)               FreeLibrary(handle)
#define LOAD_SYM(handle, fname)         GetProcAddress(handle, fname)
#define LIB_HANDLE                      HINSTANCE
#define OPEN_FILE(fp, filename, mode)   do {                                            \
                                            if ((fopen_s(&fp, filename, mode)) != 0) {  \
                                                fp = NULL;                              \
                                            }                                           \
                                        } while (0)
#define DLLEXPORT                       __declspec(dllexport)
#define SIGINT_RET_TYPE                 BOOL WINAPI
#define SIGINT_PARAM                    DWORD
#define SIGINT_RET                      return TRUE
#define SIGINT_REGISTER(cpu, function)  do {                                                                    \
                                            if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)function,TRUE)) {      \
                                                printf("[rISA]: Error. Couldn't register sigint handler.\n");   \
                                                cleanupSimulator(cpu, ECANCELED);                               \
                                            }                                                                   \
                                        } while (0)
#else // --- *nix
#define LOAD_LIB(libpath)               dlopen(libpath, RTLD_LAZY)
#define CLOSE_LIB(handle)               dlclose(handle)
#define LOAD_SYM(handle, fname)         dlsym(handle, fname)
#define LIB_HANDLE                      void*
#define OPEN_FILE(fp, filename, mode)   do {                                            \
                                            fp = fopen(filename, mode);                 \
                                        } while (0)
#define DLLEXPORT                       
#define SIGINT_RET_TYPE                 void
#define SIGINT_PARAM                    int
#define SIGINT_RET                      
#define SIGINT_REGISTER(cpu, function)  do {                                                                    \
                                            if ((signal(SIGINT,function) == SIG_ERR)) {                         \
                                                printf("[rISA]: Error. Couldn't register sigint handler.\n");   \
                                                cleanupSimulator(cpu, ECANCELED);                               \
                                            }                                                                   \
                                        } while (0)
#endif

#define DEFAULT_VIRT_MEM_SIZE   16384 // 16KB
#define DEFAULT_INT_PERIOD      500

#define ACCESS_MEM_W(virtMem, offset) (*(u32*)((u8*)virtMem + (offset)))
#define ACCESS_MEM_H(virtMem, offset) (*(u16*)((u8*)virtMem + (offset)))
#define ACCESS_MEM_B(virtMem, offset) (*(u8*)((u8*)virtMem + (offset)))

#define GET_BITS(var, pos, width)   ((var & ((((1 << width) - 1) << pos))) >> pos)
#define GET_OPCODE(instr)           GET_BITS(instr, 0, 7)
#define GET_RD(instr)               GET_BITS(instr, 7, 5)
#define GET_RS1(instr)              GET_BITS(instr, 15, 5)
#define GET_RS2(instr)              GET_BITS(instr, 20, 5)
#define GET_FUNCT3(instr)           GET_BITS(instr, 12, 3)
#define GET_FUNCT7(instr)           GET_BITS(instr, 25, 7)
#define GET_IMM_10_5(instr)         GET_BITS(instr, 25, 6)
#define GET_IMM_11_B(instr)         GET_BITS(instr, 7, 1)
#define GET_IMM_4_1(instr)          GET_BITS(instr, 8, 4)
#define GET_IMM_4_0(instr)          GET_BITS(instr, 7, 5)
#define GET_IMM_11_5(instr)         GET_BITS(instr, 25, 7)
#define GET_IMM_12(instr)           GET_BITS(instr, 31, 1)
#define GET_IMM_20(instr)           GET_BITS(instr, 31, 1)
#define GET_IMM_11_0(instr)         GET_BITS(instr, 20, 12)
#define GET_IMM_11_J(instr)         GET_BITS(instr, 20, 1)
#define GET_IMM_19_12(instr)        GET_BITS(instr, 12, 8)
#define GET_IMM_10_1(instr)         GET_BITS(instr, 21, 10)
#define GET_IMM_31_12(instr)        GET_BITS(instr, 12, 20)
#define GET_SUCC(instr)             GET_BITS(instr, 20, 4)
#define GET_PRED(instr)             GET_BITS(instr, 24, 4)
#define GET_FM(instr)               GET_BITS(instr, 28, 4)

// Tracing macro with Register type syntax
#define TRACE_R(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                             \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s x%d, x%d, x%d\n",                               \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name, cpu->instFields.rd, cpu->instFields.rs1,     \
            cpu->instFields.rs2); } } while(0)

// Tracing macro with Immediate type syntax
#define TRACE_I(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                             \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s x%d, x%d, %d\n",                                \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name, cpu->instFields.rd, cpu->instFields.rs1,     \
            cpu->immFinal); } } while(0)

// Tracing macro with Load type syntax
#define TRACE_L(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                             \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s x%d, %d(x%d)\n",                                \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name, cpu->instFields.rd, cpu->immFinal,           \
            cpu->instFields.rs1); } } while(0)

// Tracing macro with Store type syntax
#define TRACE_S(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                             \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s x%d, %d(x%d)\n",                                \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name, cpu->instFields.rs2, cpu->immFinal,          \
            cpu->instFields.rs1); } } while(0)

// Tracing macro with Upper type syntax
#define TRACE_U(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                             \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s x%d, 0x%08x\n",                                 \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name, cpu->instFields.rd,                          \
            cpu->immFinal); } } while(0)

// Tracing macro with Jump type syntax
#define TRACE_J(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                             \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s x%d, %d\n",                                     \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name, cpu->instFields.rd,                          \
            cpu->targetAddress); } } while(0)

// Tracing macro with Branch type syntax
#define TRACE_B(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                             \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s x%d, x%d, 0x%x\n",                              \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name, cpu->instFields.rs1, cpu->instFields.rs2,    \
            cpu->targetAddress); } } while(0)

// Tracing macro for FENCE
#define TRACE_FEN(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                           \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s fm:%d, pred:%d, succ:%d\n",                     \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name, cpu->immFields.fm, cpu->immFields.pred,      \
            cpu->immFields.succ); } } while(0)

// Tracing macro for Environment type syntax
#define TRACE_E(cpu, name) do { if (cpu->opts.o_tracePrintEnable) {                                             \
    fprintf(stderr, "[rISA]: <%-d cycles> : %08x:  0x%08x    %s\n",                                             \
        cpu->cycleCounter, cpu->pc, cpu->virtMem[cpu->pc/4], name); } } while(0)

typedef struct {
    u32 imm11_0  : 12;
    u32 imm4_0   : 5;
    u32 imm11_5  : 7;
    u32 imm11    : 1;
    u32 imm4_1   : 4;
    u32 imm10_5  : 6;
    u32 imm12    : 1;
    u32 imm31_12 : 20;
    u32 imm19_12 : 8;
    u32 imm10_1  : 10;
    u32 imm20    : 1;
    u32 succ     : 4;
    u32 pred     : 4;
    u32 fm       : 4;
} ImmediateFields;

typedef struct {
    u32 opcode : 7;
    u32 rd     : 5;
    u32 rs1    : 5;
    u32 rs2    : 5;
    u32 funct3 : 3;
    u32 funct7 : 7;
} InstructionFields;

typedef struct {
    u32 o_tracePrintEnable  : 1;
    u32 o_virtMemSize       : 1;
    u32 o_definedHandles    : 1;
    u32 o_timeout           : 1;
    u32 o_intPeriod         : 1;
} optFlags;

typedef struct rv32iHart{
    u32                 pc;
    u32                 regFile[32];
    u32                 IF;
    u32                 ID;
    s32                 immFinal;
    s32                 immPartial;
    ImmediateFields     immFields;
    InstructionFields   instFields;
    u32                 targetAddress;
    u32                 cycleCounter;
    u32                 *virtMem;
    u32                 virtMemSize;
    u32                 intPeriodVal;
    clock_t             startTime;
    clock_t             endTime;
    double              timeDelta;
    LIB_HANDLE          handlerLib;
    void                *mmioData;
    void                *envData;
    void                *intData;
    void (*pfnMmioHandler)  (struct rv32iHart *cpu);
    void (*pfnIntHandler)   (struct rv32iHart *cpu);
    void (*pfnEnvHandler)   (struct rv32iHart *cpu);
    void (*pfnInitHandler)  (struct rv32iHart *cpu);
    void (*pfnExitHandler)  (struct rv32iHart *cpu);
    optFlags            opts;
} rv32iHart;

typedef void (*pfnMmioHandler)(rv32iHart *cpu);
typedef void (*pfnIntHandler)(rv32iHart *cpu);
typedef void (*pfnEnvHandler)(rv32iHart *cpu);
typedef void (*pfnInitHandler)(rv32iHart *cpu);
typedef void (*pfnExitHandler)(rv32iHart *cpu);

typedef enum {
    OPT_VIRT_MEM_SIZE   = (1<<0),
    OPT_HANDLER_LIB     = (1<<1),
    OPT_HELP            = (1<<2),
    OPT_TRACING         = (1<<3),
    OPT_INTERRUPT       = (1<<4),
    OPT_UNKNOWN         = (1<<5),
    VALUE_OPTS          = (OPT_VIRT_MEM_SIZE | OPT_HANDLER_LIB | OPT_INTERRUPT)
} SimulatorOptions;

// --- RV32I Instructions ---
typedef enum {
    //     funct7         funct3       op
    SLLI = (0x0  << 10) | (0x1 << 7) | (0x13),
    SRLI = (0x0  << 10) | (0x5 << 7) | (0x13),
    SRAI = (0x20 << 10) | (0x5 << 7) | (0x13),
    ADD  = (0x0  << 10) | (0x0 << 7) | (0x33),
    SUB  = (0x20 << 10) | (0x0 << 7) | (0x33),
    SLL  = (0x0  << 10) | (0x1 << 7) | (0x33),
    SLT  = (0x0  << 10) | (0x2 << 7) | (0x33),
    SLTU = (0x0  << 10) | (0x3 << 7) | (0x33),
    XOR  = (0x0  << 10) | (0x4 << 7) | (0x33),
    SRL  = (0x0  << 10) | (0x5 << 7) | (0x33),
    SRA  = (0x20 << 10) | (0x5 << 7) | (0x33),
    OR   = (0x0  << 10) | (0x6 << 7) | (0x33),
    AND  = (0x0  << 10) | (0x7 << 7) | (0x33)
} RtypeInstructions;

typedef enum {
    //       funct3       op
    JALR   = (0x0 << 7) | (0x67),
    LB     = (0x0 << 7) | (0x3),
    LH     = (0x1 << 7) | (0x3),
    LW     = (0x2 << 7) | (0x3),
    LBU    = (0x4 << 7) | (0x3),
    LHU    = (0x5 << 7) | (0x3),
    ADDI   = (0x0 << 7) | (0x13),
    SLTI   = (0x2 << 7) | (0x13),
    SLTIU  = (0x3 << 7) | (0x13),
    XORI   = (0x4 << 7) | (0x13),
    ORI    = (0x6 << 7) | (0x13),
    ANDI   = (0x7 << 7) | (0x13),
    FENCE  = (0x0 << 7) | (0xf),
    ECALL  = (0x0 << 7) | (0x73),
    //       imm           funct3       op
    EBREAK = (0x1 << 20) | (0x0 << 7) | (0x73)
} ItypeInstructions;

typedef enum {
    //   funct3       op
    SB = (0x0 << 7) | (0x23),
    SH = (0x1 << 7) | (0x23),
    SW = (0x2 << 7) | (0x23)
} StypeInstructions;

typedef enum {
    //     funct3       op
    BEQ  = (0x0 << 7) | (0x63),
    BNE  = (0x1 << 7) | (0x63),
    BLT  = (0x4 << 7) | (0x63),
    BGE  = (0x5 << 7) | (0x63),
    BLTU = (0x6 << 7) | (0x63),
    BGEU = (0x7 << 7) | (0x63)
} BtypeInstructions;

typedef enum {
    //      op
    LUI   = (0x37),
    AUIPC = (0x17)
} UtypeInstructions;

typedef enum {
    //    op
    JAL = (0x6f)
} JtypeInstructions;
// --- RV32I Instructions ---

// Opcode to instruction-format mappings
typedef enum { R, I, S, B, U, J, Undefined } InstFormats;
extern const InstFormats OpcodeToFormat[128];

void stubMmioHandler(rv32iHart *cpu);
void stubIntHandler(rv32iHart *cpu);
void stubEnvHandler(rv32iHart *cpu);
void stubInitHandler(rv32iHart *cpu);
void stubExitHandler(rv32iHart *cpu);
void printHelp(void);
void cleanupSimulator(rv32iHart *cpu, int err);
SimulatorOptions isOption(const char *arg);
void processOptions(int argc, char** argv, rv32iHart *cpu);
void loadProgram(int argc, char **argv, rv32iHart *cpu);
void setupSimulator(int argc, char **argv, rv32iHart *cpu);