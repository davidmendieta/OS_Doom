#pragma once


#include <stdint.h>
#include "debug.h"






namespace PS2 {


    extern void init();


    // //ADDED
    // void setup_ps2_irqs();


    extern void read_ps2_data();
    extern void read_keyboard_data(uint8_t in_byte);
    extern void read_mouse_data(uint8_t in_byte);
}


// //ADDED
// extern "C" void keyboard_interrupt();
// extern "C" void mouse_interrupt();





