# rISA
A simple RISC-V ISA Simulator.

## Project features
- Functional simulation of RV32I
- Optional runtime loading of user-defined MMIO
- Simple implementation (under 1000 LOC)

## Dependencies
- cmake >= v3.10
- Some ANSI C/C++ compiler
- riscv64-unknown-elf-gcc
    - This is optional (only needed for building the included riscv examples)

## Build instructions
    
    $ cmake -Bbuild .
    $ cmake --build ./build

To build without examples
    
    $ cmake -Bbuild -DNO_EXAMPLES=1 .
    $ cmake --build ./build


