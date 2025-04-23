#include "vga.h"

#define COLOR_BLACK 0x0
#define COLOR_GREEN 0x2
#define COLOR_PURPLE 0xf

// packed list of VGA register values used to program the hardware into Mode 13h
// Mode 13h classic 320×200 resolution with 256 colors.
// WHAT THIS DOES (maybe): kernel will loop over this array and write each byte 
// to the cooresponding VGA I/O port to switch into graphics mode. Once these are 
// set, we can draw pixels. TURNS OUT WE MIGHT NOT NEED THIS SINCE CHOCOLATE DOOM 
// SUPPOSEDLY WRITES DIRECTLY TO THE BUFFER??
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

// Initialize VGA hardware
void vga_init(void) {
    // Set up VGA mode 13h (320x200 256 colors)
    write_regs(g_320x200x256);
    vga_clear_screen();
}

// Set VGA mode
void vga_set_mode(uint8_t mode) {
    if (mode == VGA_MODE_GRAPHICS) {
        write_regs(g_320x200x256);
    }
    // Add other modes as needed
}

// Set palette color
void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    // Wait for vertical retrace
    while (vga_inb(VGA_INSTAT_READ) & 0x08);
    
    // Set palette index
    vga_outb(VGA_AC_WRITE, index);
    
    // Write RGB values
    vga_outb(VGA_AC_WRITE, r >> 2);  // Scale down from 0-255 to 0-63
    vga_outb(VGA_AC_WRITE, g >> 2);
    vga_outb(VGA_AC_WRITE, b >> 2);
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

void write_regs(unsigned char *regs)
{
    unsigned i;
    vga_outb(VGA_MISC_WRITE, *regs);
    regs++;
    for(i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        vga_outb(VGA_SEQ_INDEX, i);
        vga_outb(VGA_SEQ_DATA, *regs);
        regs++;
    }
    vga_outb(VGA_CRTC_INDEX, 0x03);
    vga_outb(VGA_CRTC_DATA, vga_inb(VGA_CRTC_DATA) | 0x80);
    vga_outb(VGA_CRTC_INDEX, 0x11);
    vga_outb(VGA_CRTC_DATA, vga_inb(VGA_CRTC_DATA) & ~0x80);
    regs[0x03] |= 0x80;
    regs[0x11] &= ~0x80;
    for(i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        vga_outb(VGA_CRTC_INDEX, i);
        vga_outb(VGA_CRTC_DATA, *regs);
        regs++;
    }
    for(i = 0; i < VGA_NUM_GC_REGS; i++) {
        vga_outb(VGA_GC_INDEX, i);
        vga_outb(VGA_GC_DATA, *regs);
        regs++;
    }
    for(i = 0; i < VGA_NUM_AC_REGS; i++) {
        (void)vga_inb(VGA_INSTAT_READ);
        vga_outb(VGA_AC_INDEX, i);
        vga_outb(VGA_AC_WRITE, *regs);
        regs++;
    }
    (void)vga_inb(VGA_INSTAT_READ);
    vga_outb(VGA_AC_INDEX, 0x20);
}

void vga_test() {
    // Initialize VGA
    vga_init();
    
    // Clear the screen
    vga_clear_screen();
    
    // Test basic pixel plotting
    for (int i = 0; i < 320; i += 10) {
        vga_plot_pixel(i, 0, 15);  // White pixels at top
    }
    
    for (int i = 0; i < 200; i += 10) {
        vga_plot_pixel(0, i, 4);   // Red pixels at left
    }
    
    // Test drawing a rectangle
    for (int x = 100; x < 200; x++) {
        for (int y = 50; y < 100; y++) {
            vga_plot_pixel(x, y, 2);  // Green rectangle
        }
    }
    
    // Test palette setting
    vga_set_palette(5, 63, 0, 0);    // Bright red
    vga_set_palette(6, 0, 63, 0);    // Bright green
    vga_set_palette(7, 0, 0, 63);    // Bright blue
    
    // Draw some pixels with our custom palette colors
    for (int i = 0; i < 100; i++) {
        vga_plot_pixel(50 + i, 150, 5);  // Red line
        vga_plot_pixel(50 + i, 160, 6);  // Green line
        vga_plot_pixel(50 + i, 170, 7);  // Blue line
    }
    
    // Test mode setting
    vga_set_mode(VGA_MODE_GRAPHICS);
    
    // Draw a border around the screen
    for (int i = 0; i < 320; i++) {
        vga_plot_pixel(i, 0, 15);       // Top border
        vga_plot_pixel(i, 199, 15);     // Bottom border
    }
    for (int i = 0; i < 200; i++) {
        vga_plot_pixel(0, i, 15);       // Left border
        vga_plot_pixel(319, i, 15);     // Right border
    }
    
    // Draw some happy faces
    draw_happy_face(10, 10);
    draw_happy_face(100, 100);
    draw_happy_face(300, 150);
}