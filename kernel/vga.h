#ifndef __VGA_H
#define __VGA_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int size_t;

#define VGA_ADDRESS 0xA0000

extern void vga_init();
extern void vga_test();
extern void vga_clear_screen();
extern void vga_plot_pixel(int x, int y, unsigned short color);
extern void vga_display_image(const unsigned char* image, size_t width, size_t height);

// VGA mode setting
extern unsigned char g_320x200x256[];
extern void write_regs(const unsigned char *regs);

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

#endif // __VGA_H