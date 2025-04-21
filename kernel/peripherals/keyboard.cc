#include "debug.h"
#include "idt.h"
#include "machine.h"
#include "keyboard.h"
#include "SDL_scancode.h"

// PS/2 Controller ports
constexpr uint16_t PS2_DATA_PORT = 0x60;
constexpr uint16_t PS2_STATUS_PORT = 0x64;
constexpr uint16_t PS2_COMMAND_PORT = 0x64;

#define SDL_KEYDOWN 0x300
#define SDL_KEYUP   0x301



// Add basic scancode to ASCII mapping
static const char scancode_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static SDL_KeyEvent event_buffer[128];
static volatile int buf_head = 0;
static volatile int buf_tail = 0;

extern "C" void keyboardHandler_(void) {
    uint8_t status = inb(PS2_STATUS_PORT);
    if (status & 0x1) {  // Data available
        uint8_t scancode = inb(PS2_DATA_PORT);

        SDL_KeyEvent event;
        keyboard_to_sdl_event(scancode, &event);
        
        // Store the event in the buffer
        event_buffer[buf_head] = event;
        buf_head = (buf_head + 1) % 128;
        
        // Debug output
        Debug::printf("Key %s: scancode=%x\n", 
            event.type == SDL_KEYDOWN ? "pressed" : "released",
            scancode);
    }
    outb(0x20, 0x20);
}

int keyboard_read(char* buf, size_t count) {
    if (buf_head == buf_tail) {
        return 0; // No events available
    }
    
    SDL_KeyEvent* event = &event_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % 128;
    
    // Copy event data to buffer if space allows
    if (count >= sizeof(SDL_KeyEvent)) {
        memcpy(buf, event, sizeof(SDL_KeyEvent));
        return sizeof(SDL_KeyEvent);
    }
    
    return -1; // Buffer too small
}

void keyboard_init() {
    // 1. Disable devices
    outb(PS2_COMMAND_PORT, 0xAD);  // Disable first PS/2 port
    
    // 2. Flush the output buffer
    while ((inb(PS2_STATUS_PORT) & 1) == 1) {
        inb(PS2_DATA_PORT);
    }
    
    // 3. Set the configuration byte
    outb(PS2_COMMAND_PORT, 0x20);  // Get configuration byte
    uint8_t config = inb(PS2_DATA_PORT);
    config |= 1;    // Enable IRQ1 for keyboard
    outb(PS2_COMMAND_PORT, 0x60);  // Set configuration byte
    outb(PS2_DATA_PORT, config);
    
    // 4. Enable devices
    outb(PS2_COMMAND_PORT, 0xAE);  // Enable first PS/2 port
    
    // 5. Reset keyboard
    outb(PS2_DATA_PORT, 0xFF);
    while(inb(PS2_DATA_PORT) != 0xFA); // Wait for ACK
    
    // 6. Register interrupt handler for IRQ1 (keyboard)
    IDT::interrupt(33, (uint32_t)keyboardHandler_);
    
    Debug::printf("Keyboard initialized\n");
}

void keyboard_to_sdl_event(uint8_t scancode, SDL_KeyEvent* event) {
    bool is_release = scancode & 0x80;
    uint8_t keycode = scancode & 0x7F;
    
    event->type = is_release ? SDL_KEYUP : SDL_KEYDOWN;
    event->timestamp = 0; 
    
    // Set scancode
    event->keysym.scancode = (keycode < sizeof(ps2_to_sdl)/sizeof(ps2_to_sdl[0])) 
        ? ps2_to_sdl[keycode] 
        : SDL_SCANCODE_UNKNOWN;
    
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