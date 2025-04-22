// keyboard.h
#pragma once

#include <stddef.h>
#include "sdl_keycodes.h"

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

static const SDL_Scancode ps2_to_sdl[] = {
    /* 0x00 */ SDL_SCANCODE_UNKNOWN,
    /* 0x01 */ SDL_SCANCODE_ESCAPE,
    /* 0x02 */ SDL_SCANCODE_1,
    /* 0x03 */ SDL_SCANCODE_2,
    /* 0x04 */ SDL_SCANCODE_3,
    /* 0x05 */ SDL_SCANCODE_4,
    /* 0x06 */ SDL_SCANCODE_5,
    /* 0x07 */ SDL_SCANCODE_6,
    /* 0x08 */ SDL_SCANCODE_7,
    /* 0x09 */ SDL_SCANCODE_8,
    /* 0x0A */ SDL_SCANCODE_9,
    /* 0x0B */ SDL_SCANCODE_0,
    /* 0x0C */ SDL_SCANCODE_MINUS,
    /* 0x0D */ SDL_SCANCODE_EQUALS,
    /* 0x0E */ SDL_SCANCODE_BACKSPACE,
    /* 0x0F */ SDL_SCANCODE_TAB,
    /* 0x10 */ SDL_SCANCODE_Q,
    /* 0x11 */ SDL_SCANCODE_W,
    /* 0x12 */ SDL_SCANCODE_E,
    /* 0x13 */ SDL_SCANCODE_R,
    /* 0x14 */ SDL_SCANCODE_T,
    /* 0x15 */ SDL_SCANCODE_Y,
    /* 0x16 */ SDL_SCANCODE_U,
    /* 0x17 */ SDL_SCANCODE_I,
    /* 0x18 */ SDL_SCANCODE_O,
    /* 0x19 */ SDL_SCANCODE_P,
    /* 0x1A */ SDL_SCANCODE_LEFTBRACKET,
    /* 0x1B */ SDL_SCANCODE_RIGHTBRACKET,
    /* 0x1C */ SDL_SCANCODE_RETURN,
    /* 0x1D */ SDL_SCANCODE_LCTRL,
    /* 0x1E */ SDL_SCANCODE_A,
    /* 0x1F */ SDL_SCANCODE_S,
    /* 0x20 */ SDL_SCANCODE_D,
    /* 0x21 */ SDL_SCANCODE_F,
    /* 0x22 */ SDL_SCANCODE_G,
    /* 0x23 */ SDL_SCANCODE_H,
    /* 0x24 */ SDL_SCANCODE_J,
    /* 0x25 */ SDL_SCANCODE_K,
    /* 0x26 */ SDL_SCANCODE_L,
    /* 0x27 */ SDL_SCANCODE_SEMICOLON,
    /* 0x28 */ SDL_SCANCODE_APOSTROPHE,
    /* 0x29 */ SDL_SCANCODE_GRAVE,
    /* 0x2A */ SDL_SCANCODE_LSHIFT,
    /* 0x2B */ SDL_SCANCODE_BACKSLASH,
    /* 0x2C */ SDL_SCANCODE_Z,
    /* 0x2D */ SDL_SCANCODE_X,
    /* 0x2E */ SDL_SCANCODE_C,
    /* 0x2F */ SDL_SCANCODE_V,
    /* 0x30 */ SDL_SCANCODE_B,
    /* 0x31 */ SDL_SCANCODE_N,
    /* 0x32 */ SDL_SCANCODE_M,
    /* 0x33 */ SDL_SCANCODE_COMMA,
    /* 0x34 */ SDL_SCANCODE_PERIOD,
    /* 0x35 */ SDL_SCANCODE_SLASH,
    /* 0x36 */ SDL_SCANCODE_RSHIFT,
    
    // Arrow keys
    /* 0x48 */ SDL_SCANCODE_UP,
    /* 0x4B */ SDL_SCANCODE_LEFT,
    /* 0x4D */ SDL_SCANCODE_RIGHT,
    /* 0x50 */ SDL_SCANCODE_DOWN
};


// void keyboard_to_sdl_event(uint8_t scancode, SDL_KeyEvent* event);
 extern void keyboardHandler(uint8_t scancode);