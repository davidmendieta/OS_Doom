#ifndef MODES_H
#define MODES_H

#include <stdint.h>

// Video modes
typedef enum {
    VID_MODE8 = 0,   // 8-bit (256 colors)
    VID_MODE15 = 1,  // 15-bit (5-5-5 RGB)
    VID_MODE16 = 2,  // 16-bit (5-6-5 RGB)
    VID_MODE32 = 3,  // 32-bit (8-8-8-8 RGBA)
    VID_MODEGL = 4   // OpenGL (not supported in our kernel)
} video_mode_t;

// Video mode info
typedef struct {
    int width;
    int height;
    int bpp;
    video_mode_t mode;
} video_mode_info_t;

// Initialize video system
int V_Init(void);

// Set video mode
int V_SetMode(video_mode_t mode);

// Get current video mode
video_mode_t V_GetMode(void);

// Get pixel depth for mode
int V_GetPixelDepth(video_mode_t mode);

// Get mode info
video_mode_info_t V_GetModeInfo(video_mode_t mode);

// Set palette (for 8-bit mode)
int V_SetPalette(uint8_t* colors, int count);

// Write pixel to screen
void V_WritePixel(int x, int y, uint8_t color);

// Clear screen
void V_ClearScreen(uint8_t color);

// Update screen (flip buffers)
void V_UpdateScreen(void);

#endif // MODES_H 