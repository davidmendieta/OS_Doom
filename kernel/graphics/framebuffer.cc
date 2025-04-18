#include "framebuffer.h"
#include "vmm.h"   
#include <cstdint>
#include <cstddef>

Framebuffer screen;

void framebuffer_init(uintptr_t phys_addr,
                      uint32_t width,
                      uint32_t height,
                      uint32_t pitch,
                      uint32_t bpp) {
    screen.width  = width;
    screen.height = height;
    screen.bytes_per_line  = pitch;
    screen.bits_per_pixel    = bpp;

    // Calculate total buffer size
    size_t size = (size_t)(height) * pitch;
    // call VMMnaive mmap to map physical framebuffer into virtual memory
//     screen_fb.base = VMM::naive_mmap(size,
//                                      /*shared=*/false,
//                                      /*file=*/nullptr,
//                                      /*file_offset_or_phys=*/phys_addr,
//                                      /*user=*/false);
 }

void framebuffer_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    // Bounds check (optional in performance-critical code)
    if (x >= screen_fb.width || y >= screen_fb.height) return;

    // byte offset
    uint8_t *pixel = screen_fb.base + y * screen_fb.pitch + x * (screen_fb.bits_per_pixel / 8);
    // write pixel (assuming 32bpp)
    pixel = color;
}

