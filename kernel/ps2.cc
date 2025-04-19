#include "idt.h"
#include "ps2.h"

namespace PS2 {
volatile bool debug_on = true;

void init() {

	// checking if ps2 controller is ok
	outb(0x64, 0xAA);         
	while (!(inb(0x64) & 1)); 
	uint8_t result = inb(0x60);

	if(debug_on) Debug::printf("PS2 OK IF 0x55: 0x%x\n", result);

	// disable PS/2 port 1 and 2 so they don't send data while we configure the controller
	if(debug_on) Debug::printf("disable ports\n");
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

	// enable port 1 and 2
	if(debug_on) Debug::printf("reenabled ports\n");
	outb(0x64, 0xAE); // keyboard
	outb(0x64, 0xA8); // mouse

}

// ADDED
extern "C" void keyboard_interrupt() {
    Debug::printf("*** Keyboard interrupt!\n");
    outb(0x20, 0x20); // acknowledge PIC
}

extern "C" void mouse_interrupt() {
    Debug::printf("*** Mouse interrupt!\n");
    outb(0xA0, 0x20); // acknowledge slave PIC first
    outb(0x20, 0x20); // then master PIC
}

void setup_ps2_irqs() {
    IDT::interrupt(0x21, (uint32_t) keyboard_interrupt);
    IDT::interrupt(0x2C, (uint32_t) mouse_interrupt);
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
        default: return 0; // unhandled key
    }
}

// read a byte from data port
uint8_t read_ps2_data() {
	while (!(inb(0x64) & 1));
	return inb(0x60);
}

}