cmake_minimum_required(VERSION 3.10)

project(risa_tests)

include_directories(${RISA_DIR})
include_directories(${TESTS_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/external/minigdbstub)

add_compile_options(-Wall)
add_compile_options(-pedantic)

# Tests
add_executable(risa_tests test_args.cpp ${RISA_SRCS})
target_link_libraries(risa_tests ${CONAN_LIBS} ${CMAKE_DL_LIBS})