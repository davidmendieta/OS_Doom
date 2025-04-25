#include "vga.h"
#include "pci.h"
#include "machine.h"
#include "threads.h"

#define COLOR_BLACK 0x0
#define COLOR_GREEN 0x2
#define COLOR_PURPLE 0xf
#define COLOR_SONIC_BLUE 0x1F  // bright blue
#define COLOR_SONIC_TAN  0x36  // tan/peach
#define COLOR_SONIC_WHITE 0x3F // white
#define COLOR_SONIC_RED   0x4C // red
#define COLOR_SONIC_GREY  0x08 // grey

void write_regs(const unsigned char* regs) {
    unsigned i;

    // write miscellaneous output register
    outb(VGA_MISC_WRITE, *regs++);
    
    // write sequencer registers
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, *regs++);
    }

    // unlock CRTC registers
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);

    // write CRTC registers
    for (i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, *regs++);
    }

    // write graphics controller registers
    for (i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, *regs++);
    }

    // write attribute controller registers
    for (i = 0; i < VGA_NUM_AC_REGS; i++) {
        inb(VGA_INSTAT_READ); // reset flip-flop
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *regs++);
    }

    // lock 16-color palette and unblank display
    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}

void vga_init() {
    // find the VGA device
    PciDevice* vga = findDeviceVD(0x1234, 0x1111);
    if (!vga) {
        Debug::panic("VGA device not found!\n");
    }

    // get MMIO BARs
    uint32_t mmio_base = getBar(vga, 0); 
    uint32_t io_mmio   = getBar(vga, 2); 

    Debug::printf("VGA MMIO base: 0x%08x, IO MMIO base: 0x%08x\n",
                  mmio_base, io_mmio);

    write_regs(g_320x200x256);
}