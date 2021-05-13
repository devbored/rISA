cmake_minimum_required(VERSION 3.10)

project(risa_tests)

set(GTEST_BASEDIR "")
find_library(GTEST gtest HINTS ${GTEST_BASEDIR} REQUIRED)
find_library(GTEST_MAIN gtest_main HINTS ${GTEST_BASEDIR} REQUIRED)

find_package(Threads)

include_directories(${RISA_DIR})
include_directories(${TESTS_DIR})
include_directories(${ARGPARSE_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/external/minigdbstub)

add_compile_options(-Wall)
add_compile_options(-pedantic)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(risa_tests test_args.cpp ${RISA_SRCS})
target_link_libraries(risa_tests ${GTEST} ${GTEST_MAIN} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})