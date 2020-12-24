cmake_minimum_required(VERSION 3.10)

set(CMAKE_SYSTEM_NAME      Generic)
set(CROSS_COMPILE          riscv64-unknown-elf-)
set(CMAKE_C_COMPILER       "${CROSS_COMPILE}gcc")
set(CMAKE_OBJCOPY          "${CROSS_COMPILE}objcopy")
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

set(CMAKE_C_FLAGS_DEBUG     "" CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG   "" CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE   "" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "" CACHE STRING "")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -march=rv64i")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -nostartfiles")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -nostdlib")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}" CACHE STRING "CFLAGS")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "")

set(LINKER_SCRIPT          "${CMAKE_SOURCE_DIR}/basic.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${LINKER_SCRIPT}")

project(examples)
enable_language(ASM)
add_executable(basic1 basic1.s)

add_custom_command(
    TARGET basic1 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary basic1 basic1.hex
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building example programs"
)
