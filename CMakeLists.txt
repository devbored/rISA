cmake_minimum_required(VERSION 3.10)

project(risa)
set(RISA_DIR ${CMAKE_SOURCE_DIR}/src)
set(GDBSTUB_DIR ${CMAKE_SOURCE_DIR}/external/minigdbstub)
set(ARGPARSE_DIR ${CMAKE_SOURCE_DIR}/external/miniargparse)
set(HANDLER_DIR ${CMAKE_SOURCE_DIR}/examples/risa_handler)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)
# Example riscv program
set(SIMPLE_DIR ${CMAKE_SOURCE_DIR}/examples/hello_world)
set(SIMPLE_DIR_B ${CMAKE_BINARY_DIR}/examples/hello_world)

# CMake generator for RISC-V cross-compiler
set(CROSS_GEN)
# RISC-V GCC toolchain prefix triplet
set(RISCV_TC_TRIPLET)

option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_PROGS "Build example RISC-V programs" OFF)
option(GDBLOG "Build with gdb packet log printing" OFF)
if(GDBLOG)
    add_definitions(-DGDBLOG)
endif(GDBLOG)

set(RISA_SRCS
    ${RISA_DIR}/risa.c
    ${RISA_DIR}/gdbserver.c
    ${RISA_DIR}/socket.c
    ${RISA_DIR}/handlers.c
)

add_executable(risa ${RISA_DIR}/main.c ${RISA_SRCS})
if (WIN32 OR MINGW)
    target_link_libraries(risa PRIVATE wsock32 ws2_32)
else ()
    target_link_libraries(risa ${CMAKE_DL_LIBS})
endif()

target_include_directories(
    risa
    PUBLIC
    ${GDBSTUB_DIR}
    ${ARGPARSE_DIR}
    ${RISA_DIR}
)
target_compile_options(
    risa
    PUBLIC
    -Wall
    -pedantic
)
set_property(
    TARGET risa
    PROPERTY C_STANDARD 99
)

if(BUILD_TESTS)
    add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif(BUILD_TESTS)
add_subdirectory(${CMAKE_SOURCE_DIR}/examples/risa_handler)

# Example RISC-V C program uses separate cross-compiler
# (Needs to run as separate CMake command due to this)
if(BUILD_PROGS)
    if(NOT DEFINED CROSS_GEN)
        set(CROSS_GEN "Unix Makefiles")
    endif()
    if(NOT DEFINED RISCV_TC_TRIPLET)
        set(RISCV_TC_TRIPLET "riscv64-unknown-elf")
    endif()
    # Simple test program
    if (DOCKER)
        add_custom_target(PROG_SIMPLE ALL
            COMMAND docker run -v ${CMAKE_CURRENT_SOURCE_DIR}:/src -w /src riscv-gnu-toolchain
                cmake examples/hello_world
                    -Bbuild/examples/hello_world
                    -G${CROSS_GEN}
                    -DRISCV_TC_TRIPLET=${RISCV_TC_TRIPLET}
            COMMAND docker run -v ${CMAKE_CURRENT_SOURCE_DIR}:/src -w /src riscv-gnu-toolchain
                cmake --build build/examples/hello_world
        )
    else()
        add_custom_target(PROG_SIMPLE ALL
            COMMAND cmake ${SIMPLE_DIR} -B${SIMPLE_DIR_B} -G ${CROSS_GEN} -DRISCV_TC_TRIPLET=${RISCV_TC_TRIPLET}
            COMMAND cmake --build ${SIMPLE_DIR_B}
        )
    endif(DOCKER)
endif(BUILD_PROGS)
