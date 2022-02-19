#include <stdio.h>
#include "risa.h"

int main(int argc, char** argv) {
    // Init simulator
    rv32iHart_t cpu = {0};
    int err = setupSimulator(argc, argv, &cpu);
    if (err) { return err; }
    cpu.handlerProcs[RISA_INIT_HANDLER_PROC](&cpu);
    // Run
    err = executionLoop(&cpu);
    return err;
}
