
#pragma once

#include <cstdint>
#include <cstddef>

// frame buffer
struct Framebuffer {
    void *base;       // mapped virtual address
    uint32_t width;      // horizontal in pixels
    uint32_t height;     // vertical in pixels
    uint32_t bytes_per_line;      // bytes per line
    uint32_t bits_per_pixel;        // bits per pixel
};

// global framebuffer instance
extern Framebuffer screen;

// initialize framebuffer by mapping the physical region
// phys_addr : physical base address of the FB
// width, height, pitch, bpp : geometry parameters
void framebuffer_init(uintptr_t phys_addr,
                      uint32_t width,
                      uint32_t height,
                      uint32_t pitch,
                      uint32_t bits_per_pixel);

// draw a single pixel at (x,y) with 0xAARRGGBB color
inline void framebuffer_draw_pixel(uint32_t x, uint32_t y, uint32_t color);


