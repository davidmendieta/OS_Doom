#pragma once

#include <stdint.h>

// Mouse button state definitions
#define SDL_BUTTON_LEFT    1
#define SDL_BUTTON_MIDDLE  2
#define SDL_BUTTON_RIGHT   3

// Mouse event types
#define SDL_MOUSEMOTION    0x400
#define SDL_MOUSEBUTTONDOWN 0x401
#define SDL_MOUSEBUTTONUP  0x402

// Mouse button event structure
struct SDL_MouseButtonEvent {
    uint32_t type;       // SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP
    uint32_t timestamp;
    uint8_t button;      // The mouse button index (SDL_BUTTON_LEFT, etc.)
    uint8_t state;       // SDL_PRESSED or SDL_RELEASED
    uint16_t x;          // X coordinate, relative to window
    uint16_t y;          // Y coordinate, relative to window
};

// Mouse motion event structure
struct SDL_MouseMotionEvent {
    uint32_t type;       // SDL_MOUSEMOTION
    uint32_t timestamp;
    uint8_t state;       // Bitmask of button states
    uint16_t x;          // X coordinate, relative to window
    uint16_t y;          // Y coordinate, relative to window
    int16_t xrel;        // Relative motion in the X direction
    int16_t yrel;        // Relative motion in the Y direction
};