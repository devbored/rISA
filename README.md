# rISA
A simple RISC-V ISA Simulator.

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
    - A RISC-V compiler/cross-compiler for `test_program` (Optional)
- GoogleTest - for building unit tests (Optional)
    - C++11 or newer (Optional)

## Build instructions
This repo uses git submodules - make sure to pull those first:

    $ git submodule init && git submodule update

If you want to only build rISA:

    $ cmake . -Bbuild
    $ cmake --build build

If you want to build rISA with the example RISC-V `test_program`:

    $ cmake -DBUILD_RISCV_PROGRAM=ON . -Bbuild
    $ cmake --build build
    
## Building rISA with unit tests
[GoogleTest](https://github.com/google/googletest) is used as the unit testing framework. There are
a couple of different ways to build the tests:

If GoogleTest libs are already installed in a default system location:

    $ cmake -DBUILD_TESTS=ON . -Bbuild
    $ cmake --build build

If GoogleTest libs are already installed in a non-default system location:

    $ cmake -DBUILD_TESTS=ON -DGTEST_BASEDIR=<path_to_gtest_libs> . -Bbuild
    $ cmake --build build

## Building rISA with unit tests (with Conan)
If you don't have the gtest libs already installed, [Conan](https://docs.conan.io/en/latest/installation.html) can be
used to pull gtest in, then build with CMake.

Conan is cross-platform and implemented in Python - to install Conan:

    $ pip install conan

Use Conan to pull the gtest package and build with CMake:

    $ conan install -if build conanfile.txt
    $ cmake -DBUILD_TESTS=ON . -Bbuild
    $ cmake --build build

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
dynamic library as a command-line argument to rISA. This repo comes with example handlers 
(in the `examples/example_handlers` folder) which serves as a good example of how a user can write their own 
handlers as well as identify which header files are needed.

The cpu simulation object also contains an opaque user-data pointer:
```c
    void *handlerData;
```

The user-provided handler function(s) library will be able to utilize this data pointer to store
runtime information to the cpu object via this pointer (e.g. MMIO address ranges, trace info, etcetera). 
