#include "idt.h"
#include "ps2.h"
#include "smp.h"
#include "mouse.h"
#include "keyboard.h"

namespace PS2 {
volatile bool debug_on = true;

void init() {

	// checking if ps2 controller is ok
	outb(0x64, 0xAA);         
	while (!(inb(0x64) & 1)); 
	uint8_t result = inb(0x60);

	if(debug_on) Debug::printf("PS2 OK IF 0x55: 0x%x\n", result);

	// disable PS/2 port 1 and 2 so they don't send data while we configure the controller (disable interrupts)
	if(debug_on) Debug::printf("disable ps2 port interrupts\n");
	outb(0x64, 0xAD);
    outb(0x64, 0xA7);

	// if buffer full, we flush it, QEMU doesn't guarntee it isn't full
	if(debug_on) Debug::printf("flush buffer\n");
    if (inb(0x64) & 0x01) {
		inb(0x60);
	}
	
	// get configuration byte then wait until output buffer full to read config from Data Port
	if(debug_on) Debug::printf("read config\n");
    outb(0x64, 0x20); 
	while (!(inb(0x64) & 0x01));
	uint8_t config = inb(0x60);
	
	// clear IRQ1, IRQ12, translation
	/*
		Bit 0: IRQ1 (disable keyboard interrupt temporarily)
		Bit 1: IRQ12 (disable mouse interrupt)
		Bit 6: Translation (we want raw scancodes, not translated ones)
	*/
	if(debug_on) Debug::printf("modify config\n");
	// config &= ~0x43;
	config = (config | 0x03) & ~0x40; 
    
	// prepares controller for a config byte, then wait for buffer to be empty, then write modified config back to Data Port
    if(debug_on) Debug::printf("write modified config\n");
	outb(0x64, 0x60); 
	while (inb(0x64) & 0x02); 
	outb(0x60, config);

	// enable port 1 and 2 interrupts
	if(debug_on) Debug::printf("reenabled ps2 port interrupts\n");
	outb(0x64, 0xAE); // keyboard
	outb(0x64, 0xA8); // mouse

    // turn on mouse data reporting
    outb(0x64, 0xD4); // next byte goes to mouse
    outb(0x60, 0xF4); // cmd 0xF4 = enable data reporting

    // wait for mouse acknowledgement
    while (!(inb(0x64) & 1));
    uint8_t mouse_ack = inb(0x60);
    Debug::printf("mouse acknowledgement: 0x%x\n", mouse_ack); // should be 0xFA

    // add handlers to idt
    IDT::trap(32 + 1, (uint32_t)keyboard_handler_, 3);
    IDT::trap(32 + 12, (uint32_t)mouse_handler_, 3);

    

    //PIC stuff "borrowed" from Terence networking interrupts
    #define IOAPIC_REGSEL  0xFEC00000
    #define IOAPIC_WINDOW  0xFEC00010
    #define REDIR_BASE     0x10
    
    // keyboard IOPIC
    uint32_t irq_num = 1;
    uint32_t irq = irq_num;
    // We want to map the irq route #11 (or whatever irq is: this is on which line our NIC sends signals). From what I understand, REDIR_BASE is the start of the redirection table (mapping interrupt requests to an index in the IDT). There are 2 entries per IRQ (one low and one high), which explains why we add 2*irq.
    uint32_t redir_low = REDIR_BASE + 2 * irq;
    uint32_t redir_high = redir_low + 1;
    uint8_t vector = 32 + irq_num; // If we keep it this way for all devices, we shouldn't run into conflicts
    uint8_t dest_apic_id = 0; // This corresponds to which CPU is going to handle our NIC interrupts

    volatile uint32_t* sel = (volatile uint32_t*)IOAPIC_REGSEL;
    volatile uint32_t* win = (volatile uint32_t*)IOAPIC_WINDOW;

    // Set destination CPU
    *sel = redir_high;
    *win = ((uint32_t)dest_apic_id) << 24;

    // Unmask and set vector
    uint32_t flags = vector & 0xFF; // bits 0-7 = vector
    *sel = redir_low;
    *win = flags;

    // mouse IOPIC
    irq_num = 12;
    irq = irq_num;
    // We want to map the irq route #11 (or whatever irq is: this is on which line our NIC sends signals). From what I understand, REDIR_BASE is the start of the redirection table (mapping interrupt requests to an index in the IDT). There are 2 entries per IRQ (one low and one high), which explains why we add 2*irq.
    redir_low = REDIR_BASE + 2 * irq;
    redir_high = redir_low + 1;
    vector = 32 + irq_num; // If we keep it this way for all devices, we shouldn't run into conflicts
    dest_apic_id = 0; // This corresponds to which CPU is going to handle our NIC interrupts

    sel = (volatile uint32_t*)IOAPIC_REGSEL;
    win = (volatile uint32_t*)IOAPIC_WINDOW;

    // Set destination CPU
    *sel = redir_high;
    *win = ((uint32_t)dest_apic_id) << 24;

    // Unmask and set vector
    flags = vector & 0xFF; // bits 0-7 = vector
    *sel = redir_low;
    *win = flags;
    
}

char scancode_to_ascii(uint8_t scancode) {
	switch (scancode) {

        // numbers (top row)
        case 0x16: return '1';
        case 0x1E: return '2';
        case 0x26: return '3';
        case 0x25: return '4';
        case 0x2E: return '5';
        case 0x36: return '6';
        case 0x3D: return '7';
        case 0x3E: return '8';
        case 0x46: return '9';
        case 0x45: return '0';

        // letters
        case 0x1C: return 'a';
        case 0x32: return 'b';
        case 0x21: return 'c';
        case 0x23: return 'd';
        case 0x24: return 'e';
        case 0x2B: return 'f';
        case 0x34: return 'g';
        case 0x33: return 'h';
        case 0x43: return 'i';
        case 0x3B: return 'j';
        case 0x42: return 'k';
        case 0x4B: return 'l';
        case 0x3A: return 'm';
        case 0x31: return 'n';
        case 0x44: return 'o';
        case 0x4D: return 'p';
        case 0x15: return 'q';
        case 0x2D: return 'r';
        case 0x1B: return 's';
        case 0x2C: return 't';
        case 0x3C: return 'u';
        case 0x2A: return 'v';
        case 0x1D: return 'w';
        case 0x22: return 'x';
        case 0x35: return 'y';
        case 0x1A: return 'z';

        // others
        case 0x29: return ' ';  // spacebar
        case 0x5A: return '\n'; // enter
        case 0x66: return '\b'; // backspace
		case 0x14: 
            Debug::printf("left control"); // left control
            return '_';
        case 0x0D: return '\t'; // tab
		case 0x49: return '.'; // period
		case 0x41: return ','; // comma
        case 0x76: 
            Debug::printf("escape"); // escape
            return '_';
		case 0x12:
            Debug::printf("left shift"); // left shift
            return '_';
        case 0x59: 
			Debug::printf("right shift"); // right shift
			return '_';
		case 0x11: 
			Debug::printf("alt"); // alt
			return '_';

		// default
        default: return '_'; // unhandled key
    }
}



uint8_t mouse_cycle = 0;
uint8_t mouse_bytes[3];
uint32_t mouse_x_pos = 0;
uint32_t mouse_y_pos = 0;
/*
mouse_bytes[0] = flags from mouse from OSDev ps2 mouse pg
mouse_bytes[1] = x axis
mouse_bytes[2] = y axis

use "mouse_move x y" to simulate mouse coordinates

0, 0 is a special case, it 0 outs both within the input packet bits
(output shows both raw and "displaced" y coordinates)

I think the maxes for single output are <128, since byte size is 8bits, 
if u try x and y >= 128 u get multiple outputs that add up to ur input

y axis, starts from 256 and decreases when you put it into terminal. 
ie. running "mouse_move 100 100" gives u x = 100, raw y = 156, actual y = 100,
because qemu goes from its current position to (100,100) and sends a single 
X = +100, Y = –100 packet (raw bytes 0x64,0x9C) and the PS/2 device never 
actaully holds “(100,100)” itself, only those raw delta bytes.


use "mouse_button 0-7" to simulate left, middle, right mouse presses
*/

bool DEMO = true;
bool new_code = true;
bool caps = false;

bool is_letter (char c) {
    return (c >= 97) && (c <= 122);
}

void demo_mode() {
    uint8_t scancode = inb(0x60);
    if (scancode == 0xF0) {
        new_code = true;
    } else if (new_code) {
        char c = scancode_to_ascii(scancode);
        if (scancode == 0x58) {
            caps = !caps;
        }
        if (c != '_') {
            if (caps && is_letter(c)) {
                Debug::printf("%c", c - 32);
            } else {
                Debug::printf("%c", c);
            }
        }
        new_code = false;
    }
}

void debug_mode() {
    uint8_t scancode = inb(0x60);
    if (scancode == 0xF0) {
        new_code = true;
        uint8_t actual_code = inb(0x60);
        Debug::printf("KEY RELEASE: %c\n", scancode_to_ascii(actual_code));
    } else if (new_code) {
        Debug::printf("KEY PRESS: %c\n", scancode_to_ascii(scancode));
        new_code = false;
    }
    // Debug::printf("Char: %c\n", scancode_to_ascii(scancode));
    Debug::printf("Scancode: 0x%x\n", scancode);
    //mouse_cycle = 0;
}


extern "C" void keyboard_handler() {
    //Debug::printf("*** Keyboard interrupt!\n");
    SMP::eoi();
    // reset Lapic_eoi
    *((volatile uint32_t*)0xFEE000B0) = 0;

    if (DEMO) {
        demo_mode();
    } else {
        debug_mode();
    }

    

}


extern "C" void mouse_handler() {
    //Debug::printf("*** Mouse interrupt!\n");
    // Debug::printf("mouse cycle: %d\n", mouse_cycle);
    SMP::eoi();
    // reset Lapic_eoi
    *((volatile uint32_t*)0xFEE000B0) = 0;

    uint8_t mouse_byte = inb(0x60);
    mouse_bytes[mouse_cycle++] = mouse_byte;
    // Debug::printf("mouse cycle: %d\n", mouse_cycle);
    if (mouse_cycle == 3)  {
       // Debug::printf("full mouse input detected\n");

        mouse_cycle = 0;
        if ((mouse_bytes[2] & 0x01) != 0) {
            Debug::printf("left button pressed\n");
        }
        if ((mouse_bytes[2] & 0x04) != 0) {
            Debug::printf("middle button pressed\n");
        }
        if ((mouse_bytes[2] & 0x02) != 0) {
            Debug::printf("right button pressed\n");
        }
        
        uint8_t rawX = mouse_bytes[1];
        int displace_x = rawX;
        if (mouse_bytes[2] & 16) {
            //Debug::printf("NEGATIVE X: %d\n", mouse_bytes[1]);
            displace_x = 256 - rawX;
            displace_x *= -1;
        }
        mouse_x_pos += displace_x;

        uint8_t rawY = mouse_bytes[0];
        int displace_y = (256 - rawY);
        if (!(mouse_bytes[2] & 0x20)) {
            //Debug::printf("NEGATIVE Y: %d\n", mouse_bytes[0]);
            displace_y = rawY;
            displace_y *= -1;
        }
        mouse_y_pos += displace_y;

        // Detect the special absolute-move case (0,0)
        //bool origin = (rawX == 0 && rawY == 0);

        // origin maps to full 256
        //uint16_t y = origin ? 256 : (uint8_t)(256 - rawY);

        // Print raw values directly (0-255), so you see 156 for -100
        // Debug::printf("left: %d, middle: %d, right: %d, x: %u, y (raw): %u, y (disp): %u, abs x: %u, abs y: %u\n", (mouse_bytes[2] & 0x01), 
        //     (mouse_bytes[2] & 0x04), (mouse_bytes[2] & 0x02), rawX, rawY, y, mouse_x_pos, mouse_y_pos);

                       
// Extract button states from byte 0 (not byte 2!)
uint8_t buttons = mouse_bytes[2] & 0x07;  // First 3 bits are button states


// Extract X and Y movement
int16_t x_delta = mouse_bytes[1];
int16_t y_delta = mouse_bytes[0];


// Handle sign extension for negative movement
if (mouse_bytes[2] & 0x10) {
   x_delta |= 0xFF00;  // X sign bit is set, extend to 16 bits
}


if (mouse_bytes[2] & 0x20) {
   y_delta |= 0xFF00;  // Y sign bit is set, extend to 16 bits
}


// Y is inverted in PS/2, so negate it
y_delta = -y_delta;


// Call the mouseHandler to translate PS/2 data to SDL events
mouseHandler(buttons, x_delta, y_delta);








    }

}

}

// LEGACY STUFF, NOT USED
// // @diego i moved some of ur code to this function to make names fit better
// // i'm not sure where the return value should go so I left it as void for now
// void read_keyboard_data(uint8_t in_byte) {
//     Debug::printf("inb: %x\n", inb(0x64));
//     while (inb(0x64) & 1) {

//         uint8_t scancode = inb(0x60);
//         // if (scancode != 0xF0) {
//         //     Debug::printf("Scancode: 0x%x\n", scancode);
//         //     Debug::printf("Char: %c\n", scancode_to_ascii(scancode));
//         // }
//         Debug::printf("Char: %c\n", scancode_to_ascii(scancode));
//     }
    
//     mouse_cycle = 0;
//     // Debug::printf("full keyboard data\n");
// }

// void read_mouse_data(uint8_t in_byte) {
//     while (inb(0x64) & 1);
//     // uint8_t scancode = inb(0x60);
//     mouse_bytes[mouse_cycle++] = in_byte;
//     if (mouse_cycle < 3)  {
//         return; // wait for full packet since mouse packet is 3 bytes
//     }
//     mouse_cycle = 0;
//     // Debug::printf("full mouse input detected\n");
//     if ((mouse_bytes[0] & 0x01) != 0) {
//         Debug::printf("left button pressed\n");
//     }
//     if ((mouse_bytes[0] & 0x04) != 0) {
//         Debug::printf("middle button pressed\n");
//     }
//     if ((mouse_bytes[0] & 0x02) != 0) {
//         Debug::printf("right button pressed\n");
//     }
//     Debug::printf("left: %d, middle: %d, right: %d, x: %d, y:%d\n", 
//         (mouse_bytes[0] & 0x01), (mouse_bytes[0] & 0x04), (mouse_bytes[0] & 0x02), mouse_bytes[1], 256 - mouse_bytes[2]);

// }
// read a byte from data port
// void read_ps2_data() {
//     uint8_t status = inb(0x64);
//     if (status & 1) {
//         uint8_t in_byte = inb(0x60);
//         if (status & 0x20) {
//             // Debug::printf("is mouse data");
//             read_mouse_data(in_byte);
//         }
//         else {
//             read_keyboard_data(in_byte);
//         }
//     }
// }