#include "ps2.h"

namespace PS2 {
volatile bool debug_on = true;

void init() {
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
	config &= ~0x43; 
    
	// prepares controller for a config byte, then wait for buffer to be empty, then write modified config back to Data Port
    if(debug_on) Debug::printf("write modified config\n");
	outb(0x64, 0x60); 
	while (inb(0x64) & 0x02); 
	outb(0x60, config);

	// enable port 1 and 2
	if(debug_on) Debug::printf("reenable ports\n");
	outb(0x64, 0xAE);
	outb(0x64, 0xA8);

}


}