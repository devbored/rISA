#include <stdint.h>

void main(void);

// Define the linker script variables
extern uint32_t _etext;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _sdata;
extern uint32_t _edata;

// Begin user code execution here
void _start(void)
{
    // Define ptrs to access linker script variables
    uint32_t *pre_def_vals_ptr = &_etext;
    uint32_t *data_sec_ptr = &_sdata;
    uint32_t *bss_sec_ptr;

    // Copy data from LMA to VMA in the .data section
    if (pre_def_vals_ptr != data_sec_ptr)
        for (; data_sec_ptr < &_edata;)
            *data_sec_ptr++ = *pre_def_vals_ptr++;

    // Fill zeros in the .bss section
    for (bss_sec_ptr = &_sbss; bss_sec_ptr < &_ebss;)
        *bss_sec_ptr++ = 0;

    // Jump to the main program
    main();
}

// Main program
void main(void) {
    int a = 6;
    int c = 7;

    for (int i=0; i<10; ++i) {
        c = a+i;
        a = c+i;
    }

    for(;;);
}