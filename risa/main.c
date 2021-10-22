#include <stdio.h>
#include "risa.h"

int main(int argc, char** argv) {
    // Init simulator
    int err = 0;
    rv32iHart_t cpu = {0};
    err = setupSimulator(argc, argv, &cpu);
    if (err) {
        return err;
    }

    // Run
    printf("[rISA]: Running simulator...\n\n");
    err = executionLoop(&cpu);
    return err;
}
