#include "debug.h"
#include "idt.h"
#include "machine.h"
#include "keyboard.h"
// #include "SDL_scancode.h"


// PS/2 Controller ports
constexpr uint16_t PS2_DATA_PORT = 0x60;
constexpr uint16_t PS2_STATUS_PORT = 0x64;
constexpr uint16_t PS2_COMMAND_PORT = 0x64;


#define SDL_KEYDOWN 0x300
#define SDL_KEYUP   0x301


/*
figure out how to run
test functionality
*/


static SDL_KeyEvent event_buffer[128];
static volatile int buf_head = 0;
static volatile int buf_tail = 0;


extern "C" void keyboardHandler_(void) {
    // we get the scan code (passed in from ps2.cc)
    uint8_t status = inb(PS2_STATUS_PORT);
    if (status & 0x1) {  // data available
        uint8_t scancode = inb(PS2_DATA_PORT);

        SDL_KeyEvent event;
        // update that ascii field
        keyboard_to_sdl_event(scancode, &event);
       
        // Store the event in the buffer
        event_buffer[buf_head] = event;
        buf_head = (buf_head + 1) % 128;
       
        // probably add to dooms buffer or something...


        Debug::printf("Key %s: scancode=%x\n",
            event.type == SDL_KEYDOWN ? "pressed" : "released",
            scancode);
    }
    outb(0x20, 0x20);
}


void keyboard_to_sdl_event(uint8_t scancode, SDL_KeyEvent* event) {
    Debug::printf("*** insdie of keyboard_to_sdl_event\n");

    bool is_release = scancode & 0x80;
    uint8_t keycode = scancode & 0x7F;
       Debug::printf("*** breakpoint 1\n");

    event->type = is_release ? SDL_KEYUP : SDL_KEYDOWN;
    Debug::printf("*** breakpoint 2\n");

    event->timestamp = 0;
    Debug::printf("*** breakpoint 3\n");

    
    Debug::printf("Key %s: scancode=%x\n",
        event->type == SDL_KEYDOWN ? "pressed" : "released",
        scancode);
   
    // Set scancode
    event->keysym.scancode = (keycode < sizeof(64)/sizeof(64))
        ? 'a'
        : 9999;
   
        if (keycode >= 0x02 && keycode <= 0x0D) {
            event->keysym.sym = "1234567890-="[keycode-0x02];
        }
        else if (keycode >= 0x10 && keycode <= 0x1C) {
            event->keysym.sym = "qwertyuiop[]"[keycode-0x10];
        }
        else if (keycode >= 0x1E && keycode <= 0x29) {
            event->keysym.sym = "asdfghjkl;'"[keycode-0x1E];
        }
        else if (keycode >= 0x2C && keycode <= 0x35) {
            event->keysym.sym = "zxcvbnm,./"[keycode-0x2C];
        }
   
   
    // Modifiers
    event->keysym.mod = 0;
    if (keycode == 0x2A || keycode == 0x36) event->keysym.mod |= 1; // Shift
    if (keycode == 0x1D) event->keysym.mod |= 2;  


}

