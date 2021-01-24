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