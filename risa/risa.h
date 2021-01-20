typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;

#ifdef _WIN32
#define LOAD_LIB(libpath)               LoadLibrary(libpath)
#define LOAD_SYM(handle, fname)         GetProcAddress(handle, fname)
#define LIB_HANDLE                      HINSTANCE
#define OPEN_FILE(fp, filename, mode)   do {                                            \
                                            if ((fopen_s(&fp, filename, mode)) != 0) {  \
                                                fp = NULL;                              \
                                            }                                           \
                                        } while (0)
#define DLLEXPORT                       __declspec(dllexport)
#else
#define LOAD_LIB(libpath)               dlopen(libpath, RTLD_LAZY)
#define LOAD_SYM(handle, fname)         dlsym(handle, fname)
#define LIB_HANDLE                      void*
#define OPEN_FILE(fp, filename, mode)   do {                                            \
                                            fp = fopen(filename, mode);                 \
                                        } while (0)
#define DLLEXPORT                       
#endif

#define DEFAULT_VIRT_MEM_SIZE 16384 // 16KB
#define DEFAULT_INT_PERIOD 500
#define GET_BIT(var, pos) ((var & (1 << pos)) >> pos)
#define GET_BITSET(var, pos, width) ((var & ((((1 << width) - 1) << pos))) >> pos)
#define ACCESS_MEM_W(virtMem, offset) (*(u32*)((u8*)virtMem + (offset)))
#define ACCESS_MEM_H(virtMem, offset) (*(u16*)((u8*)virtMem + (offset)))
#define ACCESS_MEM_B(virtMem, offset) (*(u8*)((u8*)virtMem + (offset)))
#define DEBUG_PRINT(cpu, ...)   do {                                                                \
                                    if (cpu.opts.o_debugEnable) { printf("[rISA]: " __VA_ARGS__); } \
                                } while (0)


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
    u32 o_debugEnable       : 1;
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
    u32                 virtMemRange;
    u32                 intPeriodVal;
    u64                 timeoutVal;
    clock_t             startTime;
    clock_t             endTime;
    double              timeDelta;
    void (*pfnMmioHandler)(struct rv32iHart cpu);
    void (*pfnIntHandler)(struct rv32iHart cpu);
    void (*pfnEnvHandler)(struct rv32iHart cpu);
    optFlags            opts;
} rv32iHart;

typedef void (*pfnMmioHandler)(rv32iHart cpu);
typedef void (*pfnIntHandler)(rv32iHart cpu);
typedef void (*pfnEnvHandler)(rv32iHart cpu);
typedef void (*pfnDbgPrint)(rv32iHart cpu);

typedef enum {
    OPT_VIRT_MEM_SIZE   = (1<<0),
    OPT_HANDLER_LIB     = (1<<1),
    OPT_HELP            = (1<<2),
    OPT_DEBUG           = (1<<3),
    OPT_TIMEOUT         = (1<<4),
    OPT_INTERRUPT       = (1<<5),
    OPT_UNKNOWN         = (1<<6),
    OPT_VALUE_OPTS      = (OPT_VIRT_MEM_SIZE | OPT_HANDLER_LIB | OPT_TIMEOUT | OPT_INTERRUPT)
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
const InstFormats OpcodeToFormat [128] = { // TODO: Breaks One-Definition-Rule, find better place for this
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