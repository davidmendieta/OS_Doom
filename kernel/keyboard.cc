#include "debug.h"
#include "idt.h"
#include "machine.h"
#include "keyboard.h"
#include "ps2.h"




// PS/2 Controller ports
constexpr uint16_t PS2_DATA_PORT = 0x60;
constexpr uint16_t PS2_STATUS_PORT = 0x64;
constexpr uint16_t PS2_COMMAND_PORT = 0x64;


#define SDL_KEYDOWN 0x300
#define SDL_KEYUP   0x301


// static SDL_KeyEvent event_buffer[128];
// static volatile int buf_head = 0;
// static volatile int buf_tail = 0;


// extern "C" void keyboardHandler_(void) {
//     // Get the scan code
//     uint8_t status = inb(PS2_STATUS_PORT);
//     if (status & 0x1) {  // data available
//         uint8_t scancode = inb(PS2_DATA_PORT);
       
//         // Process the scancode
//         keyboardHandler(scancode);
//     }
   
//     // Send EOI to PIC
//     outb(0x20, 0x20);
// }


 void keyboardHandler(uint8_t scancode) {
    bool is_release = scancode & 0x80;
    uint8_t keycode = scancode & 0x7F;
   
    // Determine the SDL scancode
    SDL_Scancode sdl_scancode = (keycode < sizeof(ps2_to_sdl)/sizeof(ps2_to_sdl[0]))
        ? ps2_to_sdl[keycode]
        : SDL_SCANCODE_UNKNOWN;
   
    // determine the ascii value
    char ascii_val = PS2::scancode_to_ascii(scancode);
    int sym = (int) ascii_val;


    // determine modifier keys
    uint16_t mod = 0;
    if (keycode == 0x2A || keycode == 0x36) mod |= 1; // Shift
    if (keycode == 0x1D) mod |= 2; // Ctrl
   
    // create the event with all fields
    SDL_KeyEvent event = {
        static_cast<uint32_t>(is_release ? SDL_KEYUP : SDL_KEYDOWN),  // type
        0,                                      // timestamp
        {                                       // keysym
            sdl_scancode,                       // scancode
            sym,                                // sym
            mod,                                // mod
            0                                   // unused
        }
    };
   
    // Store the event in the buffer
    // event_buffer[buf_head] = event;
    // buf_head = (buf_head + 1) % 128;
   
    Debug::printf("Key %s: scancode=%x, ascii=%c\n",
        event.type == SDL_KEYDOWN ? "pressed" : "released",
        scancode,
        (sym > 0 && sym < 128) ? sym : '?');
}