#include <stdint.h>

#include "sys.h"
#include "stdint.h"
#include "debug.h"
#include "idt.h"
#include "machine.h"
#include "sdl.h"
#include "vga.h"
#include <heap.h>

// Color management
int SDL_SetPalette(SDL_Surface *surface, int flags, SDL_Color *colors, int firstcolor, int ncolors);

// 2. Surface management
SDL_Surface *SDL_Set_Video_Mode(int width, int height, int bpp, uint32_t flags)
{
    // THIS SHOULD BE A SYSTEM CALL
    // Allocate memory for pixel data
    // Set up surface structure
    // Return surface pointer

    if (width == 0 || height == 0)
    {
        return nullptr;
    }

    SDL_Surface *surf = new SDL_Surface;

    surf->w = width;
    surf->h = height;

    // Is bpp bits per pixel
    surf->pitch = (width * bpp) / 8;

    surf->pixels = malloc(height * surf->pitch);

    surf->flags = SDL_SWSURFACE;

    return surf;
}



void SDL_Flip(SDL_Surface *surface)
{
    // Swap front and back buffers if double buffering
    // Copy to framebuffer if not
    // Return success/failure

    // Mode 13h video memory is 200 rows of 320 bytes

    if (surface == nullptr)
    {
        Debug::panic("SURFACE IS NULLPTR");
    }

    unsigned char *vga_buffer = (unsigned char *)VGA_ADDRESS;

    for (uint32_t i = 0; i < surface->h; i++)
    {
        memcpy(vga_buffer, (unsigned char *)surface->pixels, surface->pitch);

        // Next row
        vga_buffer += 320;
    }
}


// goal of this is that given an RBG value from Doom, we can match that RGB value to the 
// closest of the 256 colors available through legacy VGA
// inline uint8_t rgb_to_vga_index(uint8_t r, uint8_t g, uint8_t b) {
//     uint32_t best_dist = UINT32_MAX; // initialize this to be the furthest possible distance
//     uint8_t bext_ind = 0;
//     for (int i = 0; i < 256; i++) {

//         // computes the squared diff 
//         int distR = (int)r - (int)vga_palette[i].red; // replace "vga_palette" with whatever the name of the 2d arr pallette is
//         int distG = (int)g - (int)vga_palette[i].green;
//         int distB = (int)b - (int)vga_palette[i].blue;
//         uint32_t dist = distR*distR + distG*distG + distB*distB;

//         // if this distance is smaller than any seen before, record it and save i as best_ind
//         if(dist < best_dist) {
//             best_dist = dist;
//             bext_ind  = (uint8_t)i;
//         }

//         return bext_ind;
//     }
// }

