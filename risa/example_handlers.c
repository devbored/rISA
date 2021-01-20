#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "risa.h"

DLLEXPORT void risaMmioHandler(rv32iHart cpu) {
    DEBUG_PRINT(cpu, "MMIO STUB PRINT\n");
    return;
}
DLLEXPORT void risaIntHandler(rv32iHart cpu) {
    DEBUG_PRINT(cpu, "INTERRUPT STUB PRINT\n");
}
DLLEXPORT void risaEnvHandler(rv32iHart cpu) {
    DEBUG_PRINT(cpu, "ENVIRONMENT STUB PRINT\n");
    return;
}