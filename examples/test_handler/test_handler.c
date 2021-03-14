#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "risa.h"

DLLEXPORT void risaMmioHandler(rv32iHart *cpu) {
    printf("MMIO HELLO WORLD\n");
    return;
}
DLLEXPORT void risaIntHandler(rv32iHart *cpu) {
    printf("INTERRUPT HELLO WORLD\n");
}
DLLEXPORT void risaEnvHandler(rv32iHart *cpu) {
    printf("ENVIRONMENT HELLO WORLD\n");
    return;
}
DLLEXPORT void risaInitHandler(rv32iHart *cpu) {
    printf("INIT HELLO WORLD\n");
    return;
}
DLLEXPORT void risaExitHandler(rv32iHart *cpu) {
    printf("EXIT HELLO WORLD\n");
    return;
}