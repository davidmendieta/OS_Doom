#include "vga.h"
#include "pci.h"
#include "machine.h"
#include "threads.h"

#define COLOR_BLACK 0x0
#define COLOR_GREEN 0x2
#define COLOR_PURPLE 0xf

unsigned char g_320x200x256[] =
{
    0x63,
    0x03, 0x01, 0x0F, 0x00, 0x0E,
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};


void vga_clear_screen() {
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            vga_plot_pixel(x, y, COLOR_BLACK);
        }
    }
}

void vga_plot_pixel(int x, int y, unsigned short color) {
    unsigned short offset = x + 320 * y;
    unsigned char *VGA = (unsigned char*) VGA_ADDRESS;
    VGA[offset] = color;
}

void vga_display_image(const unsigned char* image, size_t width, size_t height) {
    write_regs(g_320x200x256);
    for (size_t y = 0; y < height && y < 200; y++) {
        for (size_t x = 0; x < width && x < 320; x++) {
            vga_plot_pixel(x, y, image[y * width + x]);
        }
    }
}

void draw_rectangle(int x, int y, int width, int height) {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			vga_plot_pixel(x+i, y+j, COLOR_GREEN);
		}
	}
}

void draw_happy_face(int x, int y) {
	// eye
	vga_plot_pixel(x,y,COLOR_PURPLE);
	// eye
	vga_plot_pixel(x+10,y,COLOR_PURPLE);
	// mouth
	vga_plot_pixel(x,	y+8,COLOR_PURPLE);
	vga_plot_pixel(x+1,	y+9,COLOR_PURPLE);
	vga_plot_pixel(x+2,	y+10,COLOR_PURPLE);
	vga_plot_pixel(x+3,	y+10,COLOR_PURPLE);
	vga_plot_pixel(x+4,	y+10,COLOR_PURPLE);
	vga_plot_pixel(x+5,	y+10,COLOR_PURPLE);
	vga_plot_pixel(x+6,	y+10,COLOR_PURPLE);
	vga_plot_pixel(x+7,	y+10,COLOR_PURPLE);
	vga_plot_pixel(x+8,	y+10,COLOR_PURPLE);
	vga_plot_pixel(x+9,	y+9,COLOR_PURPLE);
	vga_plot_pixel(x+10,y+8,COLOR_PURPLE);
}

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
        inb(VGA_INSTAT_READ);               // reset flip-flop
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *regs++);
    }

    // lock 16-color palette and unblank display
    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}

void vga_test() {
    vga_clear_screen();

    draw_rectangle(150, 10, 100, 50);
	// // draw some faces
	draw_happy_face(10,10);
	draw_happy_face(100,100);
	draw_happy_face(300,150);
	// bounds
	vga_plot_pixel(0, 0, 15);
	vga_plot_pixel(319, 199, COLOR_PURPLE);
	// see some colors
	for (int i = 0; i < 15; i++) {
		for (int j = 0; j < 100; j++) {
			vga_plot_pixel(i, 50+j, i);
		}
	}
    for (int i = 16; i < 31; i++) {
		for (int j = 0; j < 100; j++) {
			vga_plot_pixel(i, 50+j, i);
		}
	}

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
    sleep(2);
    vga_test();
    sleep(10);
}