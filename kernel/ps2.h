#pragma once

#include <stdint.h>
#include "debug.h"



namespace PS2 {

	extern void init();

	// //ADDED
 	// void setup_ps2_irqs();

	extern uint8_t read_ps2_data();
}

// //ADDED
// extern "C" void keyboard_interrupt();
// extern "C" void mouse_interrupt();

