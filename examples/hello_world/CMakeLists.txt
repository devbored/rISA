cmake_minimum_required(VERSION 3.10)

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_C_COMPILER       "${RISCV_TC_TRIPLET}-gcc")
set(CMAKE_OBJCOPY          "${RISCV_TC_TRIPLET}-objcopy")
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

set(CMAKE_C_FLAGS_DEBUG     "" CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG   "" CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE   "" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "" CACHE STRING "")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -march=rv32i")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -mabi=ilp32")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -g")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}" CACHE STRING "CFLAGS")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "")

set(LINKER_SCRIPT          "${CMAKE_CURRENT_SOURCE_DIR}/risa.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--defsym=RAM_LEN=0x100000")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${LINKER_SCRIPT}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=sections.map")

project(hello_world C ASM)
add_executable(hello_world
    main.c
    syscalls.c
    startup.s
)

add_custom_command(
    TARGET hello_world POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary hello_world hello_world.hex
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building example programs"
)
