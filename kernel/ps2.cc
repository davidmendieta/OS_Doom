#include "idt.h"
#include "ps2.h"
#include "smp.h"
#include "mouse.h"
#include "keyboard.h"

namespace PS2 {

static void wait_input_empty() {
    for (unsigned i = 0; i < 1000000; ++i) if (!(inb(0x64) & 2)) return;
    Debug::panic("PS/2 command timeout\n");
}
static void command(uint8_t value) { wait_input_empty(); outb(0x64, value); }
static void data(uint8_t value) { wait_input_empty(); outb(0x60, value); }
static uint8_t reply() {
    for (unsigned i = 0; i < 1000000; ++i) if (inb(0x64) & 1) return inb(0x60);
    Debug::panic("PS/2 reply timeout\n");
}
static void device_command(bool mouse, uint8_t value) {
    for (unsigned retry = 0; retry < 3; ++retry) {
        if (mouse) command(0xd4);
        data(value);
        uint8_t response = reply();
        if (response == 0xfa) return;
        if (response != 0xfe) break;
    }
    Debug::panic("PS/2 command not acknowledged\n");
}

void init() {
    command(0xad);
    command(0xa7);
    while (inb(0x64) & 1) inb(0x60);
    command(0x20);
    uint8_t config = reply() & ~0x43; // IRQs off, untranslated set 2
    command(0x60);
    data(config);
    command(0xae);
    command(0xa8);
    device_command(false, 0xf5); // disable scanning while configuring
    device_command(false, 0xf0);
    device_command(false, 2);
    device_command(false, 0xf4);
    device_command(true, 0xf6); // defaults: standard three-byte mouse packets
    device_command(true, 0xf4);
    IDT::interrupt(33, (uint32_t)keyboard_handler_);
    IDT::interrupt(44, (uint32_t)mouse_handler_);
    command(0x60);
    data((config & ~0x30) | 3); // clocks and IRQs enabled

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

// ADDED
static char kbd_buf[16];
static int  kbd_head = 0;
static int kbd_tail = 0;

bool keyboard_has_char() {
    return kbd_head != kbd_tail;
}

char keyboard_get_char() {
    // caller must ensure has_char()==true
    char c = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) & (sizeof(kbd_buf)-1);
    return c;
}

uint8_t mouse_cycle = 0;
uint8_t mouse_bytes[3];
// change to false for debug mode!!
// must be true for hangman testcase to work properly
bool DEMO = false;


bool caps = false;
char letter = '\0';
bool release_initiated;
uint8_t last_scancode;

bool is_letter (char c) {
    return (c >= 97) && (c <= 122);
}



void demo_mode() {
    uint8_t scancode = inb(0x60);
    // Debug::printf("scancode: %x\n", scancode);
    if (scancode == 0xF0) {
        release_initiated = true;

    } else {
        
        letter = scancode_to_ascii(scancode);
        last_scancode = scancode;
    }
    if (release_initiated) {
        if (last_scancode == 0x58) {
            caps = !caps;
        }
        if (letter != '_') {
            char c = letter;
            int next = (kbd_head + 1) & (sizeof(kbd_buf)-1);
            if (next != kbd_tail) {
                kbd_buf[kbd_head] = (caps && is_letter(c)) ? c-32 : c;
                kbd_head = next;
            }
            if (caps && is_letter(c)) {
                Debug::printf("%c\n", c - 32);
            } else {
                Debug::printf("%c\n", c);
            }
        }
        
        release_initiated = false;
    }
    
}

// Both IRQs share the controller data port. Route by AUX status, not by
// which interrupt happened to arrive, and never read an empty output buffer.
static void drain_input() {
    for (unsigned i = 0; i < 64; ++i) {
        uint8_t status = inb(0x64);
        if (!(status & 1)) break;
        uint8_t byte = inb(0x60);
        if (status & 0xc0) { mouse_cycle = 0; continue; }
        if (!(status & 0x20)) {
            keyboardHandler(byte);
            continue;
        }
        if (!mouse_cycle && !(byte & 8)) continue; // packet sync bit
        mouse_bytes[mouse_cycle++] = byte;
        if (mouse_cycle == 3) {
            mouse_cycle = 0;
            uint8_t flags = mouse_bytes[0];
            int dx = mouse_bytes[1] - ((flags & 0x10) ? 256 : 0);
            int dy = mouse_bytes[2] - ((flags & 0x20) ? 256 : 0);
            // Overflow invalidates motion, but button state remains useful.
            if (flags & 0xc0) dx = dy = 0;
            mouseHandler(flags & 7, dx, -dy);
        }
    }
}
extern "C" void keyboard_handler() {
    if (DEMO) demo_mode();
    else drain_input();
    SMP::eoi();
}
extern "C" void mouse_handler() {
    drain_input();
    SMP::eoi();
}
}
