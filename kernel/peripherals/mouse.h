#ifndef MOUSE_H
#define MOUSE_H
#include <stdint.h>

//not sure if we actually need all of these commands tho
//PS/2 Mouse-specific commands
#define MOUSE_RESET 0xFF
#define MOUSE_ENABLE_DATA_REPORTING 0xF4
#define MOUSE_DISABLE_DATA_REPORTING 0xF5
#define MOUSE_SET_DEFAULTS 0xF6

//Mouse packet bit masks
//useful, not sure about the rest 
#define MOUSE_MIDDLE (1 << 2)
#define MOUSE_RIGHT (1 << 1)
#define MOUSE_LEFT (1 << 0)

#define MOUSE_Y_OVERFLOW (1 << 7) //hmmmmmmmmm what does this do?
#define MOUSE_X_OVERFLOW (1 << 6)	//overflow moment
#define MOUSE_Y_NEG (1 << 5)
#define MOUSE_X_NEG (1 << 4)
#define MOUSE_ALWAYS_SET (1 << 3)

//Function declarations
void ps2_mouse_init();          // Initializes the PS/2 mouse
void mouseHandler();       		// Handles mouse interrupts and processes packets
int mouse_get_x();              // Returns the current X position of the mouse
int mouse_get_y();              // Returns the current Y position of the mouse

#endif