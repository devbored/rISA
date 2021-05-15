#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <signal.h>
#include <algorithm>

#include <gtest/gtest.h>
extern "C" { // rISA is a pure C project - prevent name mangling
    #include "risa.h"
}

TEST(risa, dummy) {
    EXPECT_EQ(1, 1);
}