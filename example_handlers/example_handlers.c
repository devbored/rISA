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
#include "risa.h" // Ordering of headers matters, make sure "risa.h" gets included last

#ifdef __cplusplus
extern "C" {
#endif
DLLEXPORT void risaMmioHandler(rv32iHart *cpu) {
    DEBUG_PRINT(cpu, "MMIO HELLO WORLD\n");
    return;
}
DLLEXPORT void risaIntHandler(rv32iHart *cpu) {
    DEBUG_PRINT(cpu, "INTERRUPT HELLO WORLD\n");
}
DLLEXPORT void risaEnvHandler(rv32iHart *cpu) {
    DEBUG_PRINT(cpu, "ENVIRONMENT HELLO WORLD\n");
    return;
}
DLLEXPORT void risaInitHandler(rv32iHart *cpu) {
    DEBUG_PRINT(cpu, "INIT HELLO WORLD\n");
    return;
}
DLLEXPORT void risaExitHandler(rv32iHart *cpu) {
    DEBUG_PRINT(cpu, "EXIT HELLO WORLD\n");
    return;
}
#ifdef __cplusplus
}
#endif