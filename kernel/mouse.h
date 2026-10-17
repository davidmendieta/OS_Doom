#pragma once


#include <stddef.h>
#include <stdint.h>
#include "ringbuffer.h"


// SDL Mouse event types
#define SDL_MOUSEMOTION      0x400
#define SDL_MOUSEBUTTONDOWN  0x401
#define SDL_MOUSEBUTTONUP    0x402


// SDL Mouse button definitions
#define SDL_BUTTON_LEFT      1
#define SDL_BUTTON_MIDDLE    2
#define SDL_BUTTON_RIGHT     3

extern RingBuffer<int16_t> buff;

void mouse_init();
extern void mouseHandler(uint8_t buttons, int16_t x_delta, int16_t y_delta);

