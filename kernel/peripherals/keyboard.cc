#include "debug.h"
#include "idt.h"
#include "machine.h"

// PS/2 Controller ports
constexpr uint16_t PS2_DATA_PORT = 0x60;
constexpr uint16_t PS2_STATUS_PORT = 0x64;
constexpr uint16_t PS2_COMMAND_PORT = 0x64;

// Add basic scancode to ASCII mapping
static const char scancode_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static char keyboard_buffer[128];
static volatile int buf_head = 0;
static volatile int buf_tail = 0;

extern "C" void keyboardHandler_(void) {
    uint8_t status = inb(PS2_STATUS_PORT);
    if (status & 0x1) {  // Data available
        uint8_t scancode = inb(PS2_DATA_PORT);
        
        // Only handle key press events (not key release)
        if (scancode < 128) {
            char c = scancode_map[scancode];
            if (c) {
                keyboard_buffer[buf_head] = c;
                buf_head = (buf_head + 1) % 128;
                Debug::printf("%c", c); // Echo character for debugging
            }
        }
    }
    
    // Send End of Interrupt to PIC
    outb(0x20, 0x20);
}

char keyboard_read() {
    while (buf_head == buf_tail) {
        // Wait for character
        asm volatile("pause");
    }
    
    char c = keyboard_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % 128;
    return c;
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