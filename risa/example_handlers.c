#include <stdio.h>
#include <stdint.h>
#include "risa.h"

void risaMmioHandler(u32 addr, u32 *virtMem, rv32iHart cpu) {
    printf("MMIO STUB PRINT\n");
    return;
}
void risaIntHandler(u32 *vertMem, rv32iHart cpu) {
    printf("INTERRUPT STUB PRINT\n");
}
void risaEnvHandler(u32 *vertMem, rv32iHart cpu) {
    printf("ENVIRONMENT STUB PRINT\n");
    return;
}