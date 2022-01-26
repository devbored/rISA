#include <stdio.h>
#include "risa.h"

int main(int argc, char** argv) {
    // Init simulator
    rv32iHart_t cpu = {0};
    int err = setupSimulator(argc, argv, &cpu);
    if (err) { return err; }

    // Run
    LOG_I("Running simulator...\n\n");
    err = executionLoop(&cpu);
    return err;
}
