#include <stdio.h>
#include "risa.h"

DLLEXPORT void risaMmioHandler(rv32iHart_t *cpu) {
    printf("MMIO HELLO WORLD\n");
    return;
}
DLLEXPORT void risaIntHandler(rv32iHart_t *cpu) {
    printf("INTERRUPT HELLO WORLD\n");
}
DLLEXPORT void risaEnvHandler(rv32iHart_t *cpu) {
    printf("ENVIRONMENT HELLO WORLD\n");
    return;
}
DLLEXPORT void risaInitHandler(rv32iHart_t *cpu) {
    printf("INIT HELLO WORLD\n");
    return;
}
DLLEXPORT void risaExitHandler(rv32iHart_t *cpu) {
    printf("EXIT HELLO WORLD\n");
    return;
}