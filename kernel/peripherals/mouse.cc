#include "mouse.h"
#include "io.h"
#include "machine.h"
#include "idt.h"

#define PS2_CMD_PORT 0x64
#define PS2_DATA_PORT 0x60

static int8_t mouse_x = 0, mouse_y = 0;

void ps2_mouse_init() {
    // enable the mouse
    outb(PS2_CMD_PORT, 0xA8);

    // Enable interrupts for the mouse
    outb(PS2_CMD_PORT, 0x20); // Read the current command byte
    uint8_t status = inb(PS2_DATA_PORT);
    status |= 0x02; // Enable IRQ12 (mouse interrupt)
    outb(PS2_CMD_PORT, 0x60); // Write the command byte
    outb(PS2_DATA_PORT, status);

    // Reset the mouse
    outb(PS2_DATA_PORT, MOUSE_RESET);
    inb(PS2_DATA_PORT); // Acknowledge
}

void ps2_mouse_handler() {
    static uint8_t packet[3];
    static uint8_t packet_index = 0;

    // Read the next byte from the mouse
    packet[packet_index++] = inb(PS2_DATA_PORT);

    if (packet_index == 3) {
        // Process the packet
        int8_t dx = packet[1];
        int8_t dy = packet[2];

        // Update mouse position
        mouse_x += dx;
        mouse_y -= dy; // Y-axis is inverted

        // Handle button states (packet[0] & 0x07)
        // ...

        packet_index = 0; // Reset for the next packet
    }
}

int mouse_get_x() {
    return mouse_x;
}

int mouse_get_y() {
    return mouse_y;
}

extern "C" void irq12_handler() {
    ps2_mouse_handler();
    outb(0x20, 0x20); // Send End of Interrupt (EOI) to the PIC
}