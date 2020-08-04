#include <stdint.h>
#include "riemu.h"

int main(void) {
	// Init the PC, emu counter, virualized IO, and memory
	// ...	

	// Run the emulator
	for (;;) {
		// Update PC and emu counter
		// ...

		// Fetch and decode the current instruction
		// ...

		// Evaluate interupts once emu counter reaches certain value
		// ...
	}

	return 0;
}
