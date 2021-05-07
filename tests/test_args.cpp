#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <signal.h>
#include <algorithm>

#include <gtest/gtest.h>
extern "C" { // rISA is a pure C project - prevent name mangling
    #include "test_common.hpp"
    #include "risa.h"
}

TEST(risa, test_isOption) {
    // Test valid options given
    std::array<std::string, 7> validOpts = {"-m", "-l", "-h", "-d", "-t", "-i", "-gdb"};
    std::array<SimulatorOptions, 7> expectedRets = {
        OPT_VIRT_MEM_SIZE,
        OPT_HANDLER_LIB,
        OPT_HELP,
        OPT_TRACING,
        OPT_TIMEOUT,
        OPT_INTERRUPT,
        OPT_GDB
    };
    for (auto it = validOpts.begin(); it != validOpts.end(); ++it) {
        auto i = std::distance(validOpts.begin(), it);
        SimulatorOptions ret = isOption(it->c_str());
        EXPECT_EQ(ret, expectedRets[i]);
    }
}