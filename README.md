# rISA
A simple RISC-V ISA Simulator.

```
 ________  ___  ________  ________
|\   __  \|\  \|\   ____\|\   __  \
\ \  \|\  \ \  \ \  \___|\ \  \|\  \
 \ \   _  _\ \  \ \_____  \ \   __  \
  \ \  \\  \\ \  \|____|\  \ \  \ \  \
   \ \__\\ _\\ \__\____\_\  \ \__\ \__\
    \|__|\|__|\|__|\_________\|__|\|__| - RISCV (RV32I) ISA simulator
                  \|_________|

[rISA]:[INFO ]:[      risa.c]:[   164]:[      setupSimulator] - Interrupt period set to: 500 cycles.
[rISA]:[INFO ]:[      risa.c]:[   165]:[      setupSimulator] - Virtual memory size set to: 1.000000 MB.
[rISA]:[INFO ]:[      risa.c]:[   178]:[       executionLoop] - Running simulator...
============================================================================================
Hello World in rISA!
============================================================================================
[rISA]:[INFO ]:[      risa.c]:[    53]:[    cleanupSimulator] - Simulation stopping, time elapsed: 0.000109 seconds.
```

## Project features
- Functional simulation of RV32I
- Cross platform (Windows, macOS, Linux)
- GDB mode to run simulator as a gdbserver
    - Feature is currently experimental
- Optional runtime loading of user-defined handlers via shared library (i.e. `dlopen`/`LoadLibrary`)
    - MMIO handler
    - Environment handler (i.e. FENCE, ECALL and EBREAK)
    - Interrupt handler

## Dependencies
- CMake (v3.10 or higher)
- Some compatible CMake Generator (e.g. GNU Make, Visual Studio, Ninja)
- Some ANSI C compiler (C99 or higher)
    - A generic ELF/Newlib GCC RISC-V compiler/cross-compiler toolcahin for "hello world" test program (Optional)
- GoogleTest - for building unit tests (Optional)
    - C++11 or newer (Optional)

## Build instructions 🔨
This repo uses git submodules - make sure to pull those first:

    $ git submodule init && git submodule update

If you want to only build rISA:

    $ cmake . -Bbuild
    $ cmake --build build

If you want to build rISA with the example RISC-V program:

    $ cmake -DBUILD_PROGS=ON . -Bbuild
    $ cmake --build build

There is a `${CROSS_GEN}` CMake config variable that one can specify to provide a build tool type for
the cross-compile (`Unix Makefiles` is the default if not set). Another CMake config variable `${RISCV_TC_TRIPLET}`
can be specified to define the GCC prefix triple for the toolchain (`riscv64-unknown-elf` is the default if not set).

Example:

    $ cmake -DCROSS_GEN="Unix Makefiles" -DRISCV_TC_TRIPLET=riscv-none-embed ...

## Cross-compile RISC-V program example with Docker 🐳
Rather than downloading a prebuilt toolchain (which may or may not be built with multilib) and configuring PATH or
building riscv64-unknown-elf toolchain from scratch, you can use the `devbored/riscv-gnu-toolchain` Docker image
to take care of this:

First time setup:

    $ docker build -t riscv-gnu-toolchain .

Then to build with Docker, define `-DDOCKER=ON` during CMake config (then build):

    $ cmake -DBUILD_PROGS=ON -DDOCKER=ON . -Bbuild
    $ cmake --build build

## Building rISA with unit tests 🧪
[GoogleTest](https://github.com/google/googletest) is used as the unit testing framework. So you will
need to have GoogleTest installed on your system for CMake to pick-up as a package.
```
cmake -DBUILD_TESTS=ON . -Bbuild
cmake --build build
```

## rISA handler functions
rISA allows for the user to define their own handler functions for dealing with either
Memory-Mapped I/O (MMIO), Environment Calls (Env), Interrupts (Int), Initialization
(Init), and the Exit handler (Exit).
```c
    void risaMmioHandler(rv32iHart *cpu);
    void risaIntHandler(rv32iHart *cpu);
    void risaEnvHandler(rv32iHart *cpu);
    void risaInitHandler(rv32iHart *cpu);
    void risaExitHandler(rv32iHart *cpu);
```
The user can define their own handler functions separately, compile them to a dynamic library, then pass the
dynamic library as a command-line argument to rISA. This repo comes with an example handler
(in the `examples/test_risa_handler` folder) that just indicates/prints that it was called.

The cpu simulation object also contains an opaque user-data pointer:
```c
    void *handlerData;
```

The user-provided handler function(s) library will be able to utilize this data pointer to store
runtime information to the cpu object via this pointer (e.g. MMIO address ranges, trace info, etcetera).
