# rISA
A simple RISC-V ISA Simulator.

## Project features
- Functional simulation of RV32I
- Optional runtime loading of user-defined handlers
    - MMIO handler
    - Environment handler (i.e. ECALL and EBREAK)
    - Interrupt handler
- Simple implementation (under 1000 LOC)

## Dependencies
- cmake (v3.10 or higher)
- Some ANSI C/C++ compiler (C99 or higher)
- riscv64-unknown-elf-gcc
    - This is optional as it's only needed for building the included riscv assembly example(s)

## Build instructions
    
    $ cmake -Bbuild .
    $ cmake --build ./build

To build without riscv assembly example(s)
    
    $ cmake -Bbuild -DNO_EXAMPLES=1 .
    $ cmake --build ./build

## rISA handler function stubs
rISA allows for the user to define their own handler function stubs for dealing with either
MMIO, Environment Calls, or Interrupts.

- `risaMmioHandler` - Handler for MMIO
- `risaIntHandler` - Handler for Interrupts
- `risaEnvHandler` - Handler for Environment Calls

The user can define their own handlers separately, compile it to a dynamic lib., then pass the
dynamic lib. as a command-line arg to rISA. This repo comes with example handlers 
(i.e. `example_handlers.c`).

## Example usage
This repo comes with a few example riscv programs along with example handler stubs.


Running the `basic1.s` program without the example handler stubs:

    $ ./risa ../examples/basic1.hex

Running the `basic1.s` program with the example Hello World handler stubs:

    $ ./risa ../examples/basic1.hex ./libexample_handlers.<so/dll>
