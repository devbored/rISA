# rISA
A simple RISC-V ISA Simulator.

## Project features
- Functional simulation of RV32I
- Optional runtime loading of user-defined handlers
    - MMIO handler
    - Environment handler (i.e. FENCE, ECALL and EBREAK)
    - Interrupt handler

## Dependencies
- CMake (v3.10 or higher)
- Some compatible CMake Generator (e.g. GNU Make, Visual Studio, Ninja)
- Some ANSI C compiler (C99 or higher)

## Build instructions (Windows, macOS, Linux)
    
    $ cmake -Bbuild .
    $ cmake --build ./build

## rISA handler functions
rISA allows for the user to define their own handler functions for dealing with either
Memory-Mapped I/O (MMIO), Environment Calls (Env), Interrupts (Int), Initialization
(Init), and the Exit handler (Exit).

    void risaMmioHandler(rv32iHart *cpu);
    void risaIntHandler(rv32iHart *cpu);
    void risaEnvHandler(rv32iHart *cpu);
    void risaInitHandler(rv32iHart *cpu);
    void risaExitHandler(rv32iHart *cpu);

The MMIO handler gets called at the end of every S-type instruction, the Env
handler gets called at the end of the FENCE, ECALL, and EBREAK instructions, and the Int handler
gets called when the interrupt period is reached. The Init handler gets called at simulation
start to perform any initialization of user-handlers if needed. The Exit handler will be called
at the beginning of simulation cleanup. None of these handlers are required to run the
simulator (a default handler will be assigned instead per unregistered handler).

The user can define their own handler functions separately, compile them to a dynamic library, then pass the
dynamic library as a command-line argument to rISA. This repo comes with example handlers 
(in the `example_handlers` folder) which serves as a good example of how a user can write their own handlers 
as well as identify which header files are needed.

The cpu simulation object contains opaque user-data member pointers
    
    void *mmioData;
    void *intData;
    void *envData;

The user-provided handler function(s) library will be able to utilize these data pointers to store
runtime information to the cpu object via these pointers (e.g. MMIO address ranges, trace info, etcetera). 
It is the users responsibility to ensure registering and calling the `risaInitHandler` to initialize any
data to these pointers, as well as registering and calling `risaExitHandler` to be able to use the
data items one last time before the simulation performs cleanup on exiting. The cleanup for the
opaque user-data items above is surface-level only (deeper freeing of resources needs to be done in `risaExitHandler`).