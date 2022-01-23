#include "stdlib.h"
#include "risa.h"

void defaultMmioHandler(rv32iHart_t *cpu)  { return; }
void defaultIntHandler(rv32iHart_t *cpu)   { return; }
void defaultExitHandler(rv32iHart_t *cpu)  { return; }
void defaultInitHandler(rv32iHart_t *cpu)  { return; }

// Provide a default simple/basic syscall handler
void defaultEnvHandler(rv32iHart_t *cpu) {
    // Detect what syscall we encountered
    switch(cpu->regFile[A7]) {
        default: break;
        case syscall_exit:
            cpu->cleanupSimulator(cpu);
            exit(0);
        // TODO: Implement "write" and "read" syscall cases next
    }
}
