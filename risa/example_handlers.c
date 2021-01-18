#include <stdio.h>
#include <stdint.h>
#include "risa.h"

DLLEXPORT void risaMmioHandler(u32 addr, u32 *virtMem, rv32iHart cpu) {
    printf("MMIO STUB PRINT\n");
    return;
}
DLLEXPORT void risaIntHandler(u32 *vertMem, rv32iHart cpu) {
    printf("INTERRUPT STUB PRINT\n");
}
DLLEXPORT void risaEnvHandler(u32 *vertMem, rv32iHart cpu) {
    printf("ENVIRONMENT STUB PRINT\n");
    return;
}