#pragma once


#include <stddef.h>
#include "sdl_keycodes.h"
#include "ringbuffer.h"


void keyboard_init();
int keyboard_read(char* buf, size_t count);




struct SDL_Keysym {
    SDL_Scancode scancode;  // use the sdl scancode values
    int sym;                // ascii
    uint16_t mod;           // modifers (shift, ctrl, etc)
    uint32_t unused;
};


struct SDL_KeyEvent {
    uint32_t type;        // SDL_KEYDOWN or SDL_KEYUP
    uint32_t timestamp;
    SDL_Keysym keysym;
};

extern RingBuffer<SDL_KeyEvent> event_buffer;

// void keyboard_to_sdl_event(uint8_t scancode, SDL_KeyEvent* event);
 extern void keyboardHandler(uint8_t scancode);



