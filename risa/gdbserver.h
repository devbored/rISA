#ifndef GDBSTUB_H
#define GDBSTUB_H

#include <stdint.h>
#include "risa.h"

void gdbserverCall(rv32iHart *cpu);
void gdbserverInit(rv32iHart *cpu);

#endif // GDBSTUB_H
