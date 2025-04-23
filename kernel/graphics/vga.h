#ifndef __VGA_H
#define __VGA_H

// Remove the kernel/debug.h include since we're not using it
// #include "../debug.h"

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int size_t;

// Rename these functions to avoid conflicts with machine.h
static inline uint8_t vga_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void vga_outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

#define VGA_ADDRESS 0xA0000
#define VGA_MEMORY VGA_ADDRESS

// VGA modes
#define VGA_MODE_GRAPHICS 0x13  // 320x200 256 colors

extern void vga_clear_screen();
extern void vga_plot_pixel(int x, int y, unsigned short color);
extern void vga_display_image(const unsigned char* image, size_t width, size_t height);
extern void vga_test();

// VGA mode setting
extern unsigned char g_320x200x256[];
extern void write_regs(unsigned char *regs);

// VGA register ports
#define VGA_AC_INDEX        0x3C0
#define VGA_AC_WRITE        0x3C0
#define VGA_AC_READ         0x3C1
#define VGA_INSTAT_READ     0x3DA
#define VGA_MISC_WRITE      0x3C2
#define VGA_MISC_READ       0x3CC
#define VGA_CRTC_INDEX      0x3D4
#define VGA_CRTC_DATA       0x3D5
#define VGA_GC_INDEX        0x3CE
#define VGA_GC_DATA         0x3CF
#define VGA_SEQ_INDEX       0x3C4
#define VGA_SEQ_DATA        0x3C5

#define VGA_NUM_AC_REGS     21
#define VGA_NUM_CRTC_REGS   25
#define VGA_NUM_GC_REGS     9
#define VGA_NUM_SEQ_REGS    5

// Additional functions needed by fb.cc
extern void vga_init(void);
extern void vga_set_mode(uint8_t mode);
extern void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

#endif // __VGA_H