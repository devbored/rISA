#include <stdio.h>
#include "risa.h"

int main(int argc, char** argv) {
    // Init simulator
    rv32iHart_t cpu = {0};
    setupSimulator(argc, argv, &cpu);

    // Run
    LOG_I("Running simulator...\n\n");
    executionLoop(&cpu);
    return 0;
}
