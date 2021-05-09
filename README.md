# rISA
A simple RISC-V ISA Simulator.

## Project features
- Functional simulation of RV32I
- Optional runtime loading of user-defined handlers via shared library (i.e. `dlopen`/`LoadLibrary`)
    - MMIO handler
    - Environment handler (i.e. FENCE, ECALL and EBREAK)
    - Interrupt handler

## Dependencies
- CMake (v3.10 or higher)
- Some compatible CMake Generator (e.g. GNU Make, Visual Studio, Ninja)
- Some ANSI C compiler (C99 or higher)
- GoogleTest - for building unit tests (Optional)

## Build instructions
If you want to only build rISA:

    $ cmake -DBUILD_TESTS=OFF . -Bbuild
    $ cmake --build build
    
## Building rISA with unit tests
[GoogleTest](https://github.com/google/googletest) is used as the unit testing framework. There are
a couple of different ways to build the tests:

If GoogleTest libs are already installed in a default system location:

    $ cmake . -Bbuild
    $ cmake --build build

If GoogleTest libs are already installed in a non-default system location:

    $ cmake -DGTEST_BASEDIR=<path_to_gtest_libs> . -Bbuild
    $ cmake --build build

## Building rISA with unit tests using Conan
If you don't have the gtest libs already installed, [Conan](https://docs.conan.io/en/latest/installation.html) can be
used to pull gtest in, then build with CMake.

Conan is cross-platform and implemented in Python - to install Conan:

    $ pip install conan

Use Conan to pull the gtest package and build with CMake:

    $ conan install -if build conanfile.txt
    $ cmake . -Bbuild
    $ cmake --build build

## rISA handler functions
rISA allows for the user to define their own handler functions for dealing with either
Memory-Mapped I/O (MMIO), Environment Calls (Env), Interrupts (Int), Initialization
(Init), and the Exit handler (Exit).

    void risaMmioHandler(rv32iHart *cpu);
    void risaIntHandler(rv32iHart *cpu);
    void risaEnvHandler(rv32iHart *cpu);
    void risaInitHandler(rv32iHart *cpu);
    void risaExitHandler(rv32iHart *cpu);

The user can define their own handler functions separately, compile them to a dynamic library, then pass the
dynamic library as a command-line argument to rISA. This repo comes with example handlers 
(in the `examples/example_handlers` folder) which serves as a good example of how a user can write their own 
handlers as well as identify which header files are needed.

## rISA opaque data items
The cpu simulation object contains opaque user-data member pointers
    
    void *mmioData;
    void *intData;
    void *envData;

The user-provided handler function(s) library will be able to utilize these data pointers to store
runtime information to the cpu object via these pointers (e.g. MMIO address ranges, trace info, etcetera). 
