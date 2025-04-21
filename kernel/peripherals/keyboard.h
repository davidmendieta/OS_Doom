// keyboard.h
#pragma once

#include <stddef.h>
#include "SDL_scancode.h"

void keyboard_init();
int keyboard_read(char* buf, size_t count);

struct SDL_Keysym {
    SDL_Scancode scancode;  // Use the SDL_SCANCODE_* values
    int sym;                // SDLK_* key symbols (ASCII for letters/numbers)
    uint16_t mod;           // Modifier keys (shift, ctrl, etc)
    uint32_t unused;
};

struct SDL_KeyEvent {
    uint32_t type;        // SDL_KEYDOWN or SDL_KEYUP
    uint32_t timestamp;
    SDL_Keysym keysym;
};

static SDL_Scancode ps2_to_sdl[] = {
    /* 0x00 */ SDL_SCANCODE_UNKNOWN,
    /* 0x01 */ SDL_SCANCODE_ESCAPE,
    /* 0x02 */ SDL_SCANCODE_1,
    /* 0x03 */ SDL_SCANCODE_2,

    /* 0x1E */ SDL_SCANCODE_A,
    /* 0x30 */ SDL_SCANCODE_B,
    /* 0x2E */ SDL_SCANCODE_C,

    /* Arrow keys */
    /* 0x48 */ SDL_SCANCODE_UP,
    /* 0x50 */ SDL_SCANCODE_DOWN,
    /* 0x4B */ SDL_SCANCODE_LEFT,
    /* 0x4D */ SDL_SCANCODE_RIGHT,
    /* Modifiers */
    /* 0x1D */ SDL_SCANCODE_LCTRL,
    /* 0x2A */ SDL_SCANCODE_LSHIFT
};


void keyboard_to_sdl_event(uint8_t scancode, SDL_KeyEvent* event);