#include "fb.h"
#include <stdlib.h>
#include <string.h>

// Current surface
static fb_surface_t* current_surface = NULL;

// Initialize framebuffer
int fb_init(int width, int height, int bpp) {
    // Initialize VGA hardware
    vga_init();
    
    // Set video mode
    if (bpp == 8) {
        vga_set_mode(VGA_MODE_GRAPHICS);  // 320x200 256 colors
    } else {
        // For now, we only support 8-bit color mode
        return -1;
    }
    
    // Create default surface
    current_surface = fb_create_surface(width, height, bpp);
    if (!current_surface) {
        return -1;
    }
    
    return 0;
}

// Create a surface
fb_surface_t* fb_create_surface(int width, int height, int bpp) {
    fb_surface_t* surface = (fb_surface_t*)malloc(sizeof(fb_surface_t));
    if (!surface) {
        return NULL;
    }
    
    // Allocate pixel data
    surface->pixels = (uint8_t*)malloc(width * height * (bpp / 8));
    if (!surface->pixels) {
        free(surface);
        return NULL;
    }
    
    surface->width = width;
    surface->height = height;
    surface->bpp = bpp;
    surface->pitch = width * (bpp / 8);
    
    return surface;
}

// Destroy a surface
void fb_destroy_surface(fb_surface_t* surface) {
    if (surface) {
        if (surface->pixels) {
            free(surface->pixels);
        }
        free(surface);
    }
}

// Set video mode
int fb_set_mode(int width, int height, int bpp) {
    if (bpp == 8) {
        vga_set_mode(VGA_MODE_GRAPHICS);  // 320x200 256 colors
        return 0;
    }
    
    // For now, we only support 8-bit color mode
    return -1;
}

// Set palette (for 8-bit mode)
int fb_set_palette(uint8_t* colors, int count) {
    for (int i = 0; i < count; i++) {
        vga_set_palette(i, colors[i*3], colors[i*3+1], colors[i*3+2]);
    }
    return 0;
}

// Write pixel to surface
void fb_write_pixel(fb_surface_t* surface, int x, int y, uint8_t color) {
    if (!surface || x < 0 || x >= surface->width || y < 0 || y >= surface->height) {
        return;
    }
    
    surface->pixels[y * surface->pitch + x] = color;
}

// Clear surface
void fb_clear_surface(fb_surface_t* surface, uint8_t color) {
    if (!surface) {
        return;
    }
    
    memset(surface->pixels, color, surface->width * surface->height * (surface->bpp / 8));
}

// Flip surface to screen
void fb_flip(fb_surface_t* surface) {
    if (!surface) {
        return;
    }
    
    // Copy surface data to VGA memory
    uint8_t* vga_mem = (uint8_t*) VGA_MEMORY; // Placeholder
    for (int y = 0; y < surface->height; y++) {
        for (int x = 0; x < surface->width; x++) {
            vga_mem[y * 320 + x] = surface->pixels[y * surface->pitch + x];
        }
    }
}

// Get current surface
fb_surface_t* fb_get_current_surface(void) {
    return current_surface;
}
