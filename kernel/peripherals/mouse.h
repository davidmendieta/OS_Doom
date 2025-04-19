#ifndef MOUSE_H
#define MOUSE_H
#include <stdint.h>
#include <stdbool.h>

// PS/2 Mouse-specific commands
#define MOUSE_RESET 0xFF
#define MOUSE_ENABLE_DATA_REPORTING 0xF4
#define MOUSE_DISABLE_DATA_REPORTING 0xF5
#define MOUSE_SET_DEFAULTS 0xF6
#define MOUSE_SET_SAMPLE 0xF3
#define MOUSE_SET_RESOLUTION 0xE8
#define MOUSE_ENABLE_SCALING 0xE7
#define MOUSE_DISABLE_SCALING 0xE6

// Mouse packet bit masks
#define MOUSE_MIDDLE (1 << 2)
#define MOUSE_RIGHT (1 << 1)
#define MOUSE_LEFT (1 << 0)

#define MOUSE_Y_OVERFLOW (1 << 7)
#define MOUSE_X_OVERFLOW (1 << 6)
#define MOUSE_Y_NEG (1 << 5)
#define MOUSE_X_NEG (1 << 4)
#define MOUSE_ALWAYS_SET (1 << 3)

// IRQ number for mouse
#define IRQ12 12

namespace mouse{

    // Function declarations
    void ps2_mouse_init();                     // Initializes the PS/2 mouse
    void mouseHandler();                       // Handles mouse interrupts and processes packets

    // Mouse position functions
    int mouse_get_x();                         // Returns the current X position of the mouse
    int mouse_get_y();                         // Returns the current Y position of the mouse

    // Mouse button state functions
    bool mouse_left_pressed();                 // Returns true if left button is pressed
    bool mouse_right_pressed();                // Returns true if right button is pressed
    bool mouse_middle_pressed();               // Returns true if middle button is pressed

    // Mouse configuration functions
    void mouse_set_sample_rate(uint8_t rate);          // Set mouse sampling rate
    void mouse_set_resolution(uint8_t resolution);     // Set mouse resolution
    void mouse_enable_data_reporting();                // Enable sending mouse data packets
    void mouse_disable_data_reporting();               // Disable sending mouse data packets
    void mouse_enable_scaling();                       // Enable mouse acceleration
    void mouse_disable_scaling();                      // Disable mouse acceleration
    void mouse_set_sensitivity(float sensitivity);     // Set mouse sensitivity multiplier

    // Debug functions
    void mouse_print_status();                 // Print current mouse position and button states

    void test_mouse();                        // Test function to display mouse movement

    // Mouse interrupt handler - this should be registered with IRQ12
    extern "C" void irq12_handler();
}

#endif // MOUSE_H