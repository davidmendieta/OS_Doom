#include "vga.h"
#include "pci.h"
#include "machine.h"
#include "threads.h"
#include "sys.h"
#include "ext2.h"
#include "kernel.h"

#define COLOR_BLACK 0x0
#define COLOR_GREEN 0x2
#define COLOR_PURPLE 0xf
#define COLOR_SONIC_BLUE 0x1F  // bright blue
#define COLOR_SONIC_TAN  0x36  // tan/peach
#define COLOR_SONIC_WHITE 0x3F // white
#define COLOR_SONIC_RED   0x4C // red
#define COLOR_SONIC_GREY  0x08 // grey

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
    for (size_t y = 0; y < height && y < 200; y++) {
        for (size_t x = 0; x < width && x < 320; x++) {
            vga_plot_pixel(x, y, image[y * width + x]);
        }
    }
}


void vga_display_file(const char* filename) {
    // // auto me = impl::threads::state.current();
    // // if(me->pcb->cwd == nullptr) me->pcb->cwd = fs->root;

    // // only read up to the buffer size
    // const size_t bufsize = 320 * 200;
    // static unsigned char buffer[320 * 200];

    // // read the file into the buffer
    // uint32_t bytes_read = open_file->node->read_all(open_file->offset, bufsize, (char*)buffer);

    // open_file->offset += bytes_read;

    // // display the image
    // vga_display_image(buffer, 320, 200);
}


void draw_rectangle(int x, int y, int width, int height, unsigned short color) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            vga_plot_pixel(x+i, y+j, color);
        }
    }
}

// draws the word SONIC in large block letters at (x, y)
void draw_sonic_banner(int x, int y) {
    int w = 18; // letter width
    int h = 30; // letter height
    int t = 4;  // thickness
    int s = 4;  // space between letters
    int color = COLOR_SONIC_BLUE;
    int cx = x;
    // S
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y, t, h/2, color); // upper left
    draw_rectangle(cx, y + h/2 - t/2, w, t, color); // middle
    draw_rectangle(cx + w - t, y + h/2, t, h/2, color); // lower right
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    cx += w + s; // add space
    // O
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx + w - t, y, t, h, color); // right
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    cx += w + s;// add space
    // N
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx + w - t, y, t, h, color); // right
    for (int i = 0; i < h; ++i) // diagonal
        draw_rectangle(cx + i * (w-t)/h, y + i, t, 1, color);
    cx += w + s; // add space
    // I
    draw_rectangle(cx + (w-t)/2, y, t, h, color); // vertical
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    cx += w + s; // add space
    // C
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    draw_rectangle(cx + w - t, y, t, t*2, color); // top right stub
    draw_rectangle(cx + w - t, y + h - t*2, t, t*2, color); // bottom right stub
}

// draw Sonic the Hedgehog at (x, y)
void draw_sonic(int x, int y) {
    // head (blue)
    draw_rectangle(x+6, y+4, 16, 16, COLOR_SONIC_BLUE);
    // face (tan)
    draw_rectangle(x+10, y+10, 8, 8, COLOR_SONIC_TAN);
    // left ear (blue)
    draw_rectangle(x+6, y+2, 4, 4, COLOR_SONIC_BLUE);
    // right ear (blue)
    draw_rectangle(x+18, y+2, 4, 4, COLOR_SONIC_BLUE);
    // spikes (blue, left and right)
    draw_rectangle(x+2, y+10, 4, 6, COLOR_SONIC_BLUE);
    draw_rectangle(x+22, y+10, 4, 6, COLOR_SONIC_BLUE);
    // eyes (white)
    draw_rectangle(x+12, y+12, 2, 3, COLOR_SONIC_WHITE);
    draw_rectangle(x+16, y+12, 2, 3, COLOR_SONIC_WHITE);
    // pupils (black)
    vga_plot_pixel(x+13, y+13, COLOR_BLACK);
    vga_plot_pixel(x+17, y+13, COLOR_BLACK);
    // nose (black)
    vga_plot_pixel(x+15, y+16, COLOR_BLACK);
    // smile (black)
    vga_plot_pixel(x+14, y+17, COLOR_BLACK);
    vga_plot_pixel(x+15, y+18, COLOR_BLACK);
    vga_plot_pixel(x+16, y+17, COLOR_BLACK);
    // body (blue)
    draw_rectangle(x+12, y+20, 8, 8, COLOR_SONIC_BLUE);
    // belly (tan)
    draw_rectangle(x+14, y+22, 4, 4, COLOR_SONIC_TAN);
    // left arm (tan)
    draw_rectangle(x+8, y+22, 4, 2, COLOR_SONIC_TAN);
    // right arm (tan)
    draw_rectangle(x+20, y+22, 4, 2, COLOR_SONIC_TAN);
    // left leg (blue)
    draw_rectangle(x+12, y+28, 2, 4, COLOR_SONIC_BLUE);
    // right leg (blue)
    draw_rectangle(x+18, y+28, 2, 4, COLOR_SONIC_BLUE);
    // left shoe (red)
    draw_rectangle(x+11, y+32, 4, 2, COLOR_SONIC_RED);
    // right shoe (red)
    draw_rectangle(x+17, y+32, 4, 2, COLOR_SONIC_RED);
    // gloves (white)
    vga_plot_pixel(x+9, y+23, COLOR_SONIC_WHITE);
    vga_plot_pixel(x+23, y+23, COLOR_SONIC_WHITE);
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
        inb(VGA_INSTAT_READ); // reset flip-flop
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *regs++);
    }

    // lock color palette and unblank display
    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}

static void short_delay() {
    volatile int wait = 0;
    for (int i = 0; i < 10000000; ++i) {
        wait += 1;
    }
}

// draws "GRAPHICS TEAM" in block letters at (x, y)
void draw_team_banner(int x, int y) {
    int w = 18; // letter width
    int h = 30; // letter height
    int t = 4;  // thickness
    int s = 4;  // space between letters
    int ws = 12; // space between words
    int color = COLOR_PURPLE;
    int cx = x;

    // G
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    draw_rectangle(cx + w - t, y + h/2, t, h/2, color); // lower right
    draw_rectangle(cx + w/2, y + h/2 - t/2, w/2, t, color); // middle right
    cx += w + s;

    // R
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx + w - t, y, t, h/2, color); // upper right
    draw_rectangle(cx, y + h/2 - t/2, w, t, color); // middle
    for (int i = 0; i < h/2; ++i) // diagonal leg
        draw_rectangle(cx + i * (w-t)/(h/2), y + h/2 + i, t, 1, color);
    cx += w + s;

    // A
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx + w - t, y, t, h, color); // right
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y + h/2 - t/2, w, t, color); // middle
    cx += w + s;

    // P
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx + w - t, y, t, h/2, color); // right
    draw_rectangle(cx, y + h/2 - t/2, w, t, color); // middle
    cx += w + s;

    // H
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx + w - t, y, t, h, color); // right
    draw_rectangle(cx, y + h/2 - t/2, w, t, color); // middle
    cx += w + s;

    // I
    draw_rectangle(cx + (w-t)/2, y, t, h, color); // vertical
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    cx += w + s;

    // C
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    draw_rectangle(cx + w - t, y, t, t*2, color); // top right stub
    draw_rectangle(cx + w - t, y + h - t*2, t, t*2, color); // bottom right stub
    cx += w + s;

    // S
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y, t, h/2, color); // upper left
    draw_rectangle(cx, y + h/2 - t/2, w, t, color); // middle
    draw_rectangle(cx + w - t, y + h/2, t, h/2, color); // lower right
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    cx += w + ws; // extra space for word separation

    // T
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx + (w-t)/2, y, t, h, color); // vertical
    cx += w + s;

    // E
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx, y + h/2 - t/2, w, t, color); // middle
    draw_rectangle(cx, y + h - t, w, t, color); // bottom
    cx += w + s;

    // A
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx + w - t, y, t, h, color); // right
    draw_rectangle(cx, y, w, t, color); // top
    draw_rectangle(cx, y + h/2 - t/2, w, t, color); // middle
    cx += w + s;

    // M
    draw_rectangle(cx, y, t, h, color); // left
    draw_rectangle(cx + w - t, y, t, h, color); // right
    for (int i = 0; i < w/2; ++i) // left diagonal
        draw_rectangle(cx + i, y + i, t, 1, color);
    for (int i = 0; i < w/2; ++i) // right diagonal
        draw_rectangle(cx + w - t - i, y + i, t, 1, color);
    cx += w + s;
}

void vga_test() {
    // vga_display_file("./output.vga");
    while (1)
    {
    // animate Sonic running from right to left
    int sonic_width = 20; // approximate width of Sonic sprite
    int y = 83; // y for middle of screen
    for (int x = 320 - sonic_width; x >= 0; x -= 4) { // move left by 4 pixels per frame
        vga_clear_screen();
        draw_sonic(x, y);
        draw_sonic_banner(100, 10);

        // delay to make animation visible  
        short_delay();
        short_delay();
    }
    }

    // while (1)
    // {
    // int y = 83; // y for middle of screen
    // for (int x = 320; x >= 0; x -= 8) { // move left by 4 pixels per frame
    //     vga_clear_screen();
    //     draw_team_banner(x, y);
        
    //     // delay to make animation visible  
    //     short_delay();
    //     short_delay();
    //     short_delay();
    //     short_delay();
    //     short_delay();
    // }
    // }
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
    sleep(1);
    vga_test();
}