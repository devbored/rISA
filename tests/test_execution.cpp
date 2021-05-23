#include <iostream>
#include <signal.h>
#include <stdlib.h>

#include <gtest/gtest.h>
extern "C" { // rISA is a pure C project - prevent name mangling
#include "risa.h"
}

TEST(risa, test_invalid_instruction) {
    rv32iHart testCPU = {0};
    testCPU.opts.o_timeout = 1;
    testCPU.timeoutVal = 1;
    testCPU.virtMem = (u32*)malloc(sizeof(u32));
    testCPU.virtMemSize = sizeof(u32);
    *testCPU.virtMem = 0xaaaaaaaa;
    
    int err = executionLoop(&testCPU);
    EXPECT_EQ(EILSEQ, err);
}

TEST(risa, test_basic_add_addi) {
    rv32iHart testCPU = {0};
    testCPU.opts.o_timeout = 1;
    testCPU.timeoutVal = 3;
    testCPU.virtMem = (u32*)malloc(sizeof(u32) * 3);
    testCPU.virtMemSize = sizeof(u32) * 3;
    *&testCPU.virtMem[0] = 0x00f00313; // addi x6 x0 15
    *&testCPU.virtMem[1] = 0x00630393; // addi x7 x6 6
    *&testCPU.virtMem[2] = 0x00638433; // add x8 x7 x6  ; Expected result: x8 = 36
    
    int err = executionLoop(&testCPU);
    EXPECT_EQ(0, err);
    EXPECT_EQ(testCPU.regFile[8], 36U);
}
