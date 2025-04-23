#ifndef FB_H
#define FB_H

#include <stdint.h>
#include "vga.h"

// Framebuffer structure
typedef struct {
    uint8_t* pixels;     // Pointer to pixel data
    int width;           // Width in pixels
    int height;          // Height in pixels
    int bpp;             // Bits per pixel
    int pitch;           // Bytes per row
} fb_surface_t;

// Initialize framebuffer
int fb_init(int width, int height, int bpp);

// Create a surface
fb_surface_t* fb_create_surface(int width, int height, int bpp);

// Destroy a surface
void fb_destroy_surface(fb_surface_t* surface);

// Set video mode
int fb_set_mode(int width, int height, int bpp);

// Set palette (for 8-bit mode)
int fb_set_palette(uint8_t* colors, int count);

// Write pixel to surface
void fb_write_pixel(fb_surface_t* surface, int x, int y, uint8_t color);

// Clear surface
void fb_clear_surface(fb_surface_t* surface, uint8_t color);

// Flip surface to screen
void fb_flip(fb_surface_t* surface);

// Get current surface
fb_surface_t* fb_get_current_surface(void);

#endif // FB_H