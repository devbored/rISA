# rISA
A simple RISC-V ISA Simulator.

## Project features
- Functional simulation of RV32I
- Optional runtime loading of user-defined handlers
    - MMIO handler
    - Environment handler (i.e. FENCE, ECALL and EBREAK)
    - Interrupt handler
- Simple implementation (under 1000 LOC)

## Dependencies
- CMake (v3.10 or higher)
- Some compatible CMake Generator (e.g. GNU make, Visual Studio, Ninja)
- Some ANSI C compiler (C99 or higher)

## Build instructions (Windows, macOS, Linux)
    
    $ cmake -Bbuild .
    $ cmake --build ./build

## rISA handler function stubs
rISA allows for the user to define their own handler functions for dealing with either
 Memory-Mapped I/O (MMIO), Environment Calls (Env), or Interrupts (Int).

    void risaMmioHandler(rv32iHart *cpu);
    void risaIntHandler(rv32iHart *cpu);
    void risaEnvHandler(rv32iHart *cpu);

The MMIO handler gets called at the end of every S-type instruction, the Env
handler gets called at the end of the FENCE, ECALL, and EBREAK instructions, and the Int handler
gets called when the interrupt period is reached.

The user can define their own handler functions separately, compile them to a dynamic library, then pass the
dynamic library as a command-line argument to rISA. This repo comes with example handlers 
(in the `example_handlers` folder) which serves as a good example of how a user can write their own handlers 
as well as identify which headers are needed.

## Usage

    [rISA]:    Usage: risa [OPTIONS] <program_binary>
               Example: risa -m 1024 my_riscv_program.hex
    
               OPTIONS:
                   -m <int>  : Virtual memory/IO size (in bytes).
                   -l <file> : Dynamic library file to user-defined handler functions.
                   -t <int>  : Simulator cycle timeout value [DEFAULT=INT32_MAX].
                   -i <int>  : Simulator interrupt-check timeout value [DEFAULT=500].
                   -h        : Print help and exit.
                   -p        : Enable debug printing.

