#include "debug.h"
#include "sdl_mouse.h"
#include "mouse.h"


// Tracking mouse state
static int16_t mouse_x = 0;
static int16_t mouse_y = 0;
static uint8_t last_buttons = 0;


void mouse_init() {
   // Initialization code if needed
   mouse_x = 0;
   mouse_y = 0;
   last_buttons = 0;
   Debug::printf("Mouse initialized, translating PS/2 to SDL events\n");
   Debug::printf("SDL Button codes: LEFT=%d, MIDDLE=%d, RIGHT=%d\n",
                SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT);
   Debug::printf("SDL Event types: MOTION=%d, BUTTONDOWN=%d, BUTTONUP=%d\n",
                SDL_MOUSEMOTION, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP);
}


void mouseHandler(uint8_t buttons, int16_t x_delta, int16_t y_delta) {
   // Print the raw PS/2 input
   Debug::printf("PS/2 raw: buttons=%02x (left=%d,middle=%d,right=%d), dx=%d, dy=%d\n",
                buttons,
                (buttons & 0x01) ? 1 : 0,
                (buttons & 0x04) ? 1 : 0,
                (buttons & 0x02) ? 1 : 0,
                x_delta, y_delta);
               
   // Update mouse position
   mouse_x += x_delta;
   mouse_y += y_delta;
  
   // Check if this is a motion event (position changed)
   if (x_delta != 0 || y_delta != 0) {
       // Translating to SDL_MOUSEMOTION event
       uint32_t sdl_event_type = SDL_MOUSEMOTION;
       uint8_t sdl_button_state = 0;
      
       // Convert PS/2 button state to SDL button state
       if (buttons & 0x01) sdl_button_state |= (1 << (SDL_BUTTON_LEFT-1));
       if (buttons & 0x04) sdl_button_state |= (1 << (SDL_BUTTON_MIDDLE-1));
       if (buttons & 0x02) sdl_button_state |= (1 << (SDL_BUTTON_RIGHT-1));
      
       Debug::printf("SDL MOUSEMOTION: type=%d, state=%d, x=%d, y=%d, xrel=%d, yrel=%d\n",
                    sdl_event_type, sdl_button_state, mouse_x, mouse_y, x_delta, y_delta);
   }
  
   // Check for button state changes
   uint8_t changed_buttons = buttons ^ last_buttons;
  
   // Check each button for changes
   for (int i = 0; i < 3; i++) {
       uint8_t ps2_mask = 0;
       uint8_t sdl_button = 0;
      
       // Map button index to PS/2 bitmask and SDL button value
       switch (i) {
           case 0: // Left button
               ps2_mask = 0x01;
               sdl_button = SDL_BUTTON_LEFT;
               break;
           case 1: // Right button (note PS/2 uses bit 1 for right)
               ps2_mask = 0x02;
               sdl_button = SDL_BUTTON_RIGHT;
               break;
           case 2: // Middle button
               ps2_mask = 0x04;
               sdl_button = SDL_BUTTON_MIDDLE;
               break;
       }
      
       if (changed_buttons & ps2_mask) {
           // Button state changed
           bool is_pressed = buttons & ps2_mask;
          
           // Show the SDL event type being created
           uint32_t sdl_event_type = is_pressed ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
           uint8_t sdl_button_state = is_pressed ? 1 : 0;
          
           Debug::printf("SDL %s: type=%d, button=%d, state=%d, x=%d, y=%d\n",
                        is_pressed ? "MOUSEBUTTONDOWN" : "MOUSEBUTTONUP",
                        sdl_event_type, sdl_button, sdl_button_state, mouse_x, mouse_y);
          
           Debug::printf("Mouse button %d %s at x=%d, y=%d\n",
                        sdl_button,
                        is_pressed ? "pressed" : "released",
                        mouse_x, mouse_y);
       }
   }
  
   // Print current state after all processing
   Debug::printf("left: %d, middle: %d, right: %d, x: %d, y:%d\n",
                buttons & 0x1, (buttons & 0x4) >> 2, (buttons & 0x2) >> 1,
                mouse_x, mouse_y);
  
   // Update last button state
   last_buttons = buttons;
}

