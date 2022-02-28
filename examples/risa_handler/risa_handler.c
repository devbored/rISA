#include <stdio.h>
#include "risa.h"

DLLEXPORT void risaMmioHandler(rv32iHart_t *cpu) {
    printf("MMIO HELLO WORLD - target address is: [ 0x%08x ]\n", cpu->targetAddress);
    return;
}
DLLEXPORT void risaIntHandler(rv32iHart_t *cpu) {
    printf("INTERRUPT HELLO WORLD - interrupt timeout is [ %d ]\n", cpu->intPeriodVal);
}
DLLEXPORT void risaInitHandler(rv32iHart_t *cpu) {
    printf("INIT HELLO WORLD\n");
    return;
}
DLLEXPORT void risaExitHandler(rv32iHart_t *cpu) {
    printf("EXIT HELLO WORLD\n");
    return;
}