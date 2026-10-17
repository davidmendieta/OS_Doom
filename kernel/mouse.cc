#include "debug.h"
#include "sdl_mouse.h"
#include "mouse.h"

// Tracking mouse state
static int16_t mouse_x = 0;
static int16_t mouse_y = 0;
static uint8_t last_buttons = 0;
static int mouse_sensitivity = 10;
static int mouse_dpi = 800;

RingBuffer<int16_t> buff (256);


void mouse_init(){
    // Initialization code if needed
    mouse_x = 0;
    mouse_y = 0;
    last_buttons = 0;
    Debug::printf("Mouse initialized, translating PS/2 to SDL events\n");
    Debug::printf("SDL Button codes: LEFT=%d, MIDDLE=%d, RIGHT=%d\n", SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT);
    Debug::printf("SDL Event types: MOTION=%d, BUTTONDOWN=%d, BUTTONUP=%d\n", SDL_MOUSEMOTION, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP);
	Debug::printf("Mouse Sensitivity: %d, DPI: %d\n", mouse_sensitivity, mouse_dpi);
}

void setMouseSensitivity(int new_sens){
    if(new_sens > 0){
        mouse_sensitivity = new_sens;
        Debug::printf("Mouse sensitivity set to %d\n", mouse_sensitivity);
    }else{
        Debug::printf("Invalid sensitivity value: %d\n", new_sens);
    }
}

void setMouseDPI(int new_dpi){
    if(new_dpi > 0){
        mouse_dpi = new_dpi;
        Debug::printf("Mouse DPI set to %d\n", mouse_dpi);
    }else{
        Debug::printf("Invalid DPI value: %d\n", new_dpi);
    }
}

void mouseHandler(uint8_t buttons, int16_t x_delta, int16_t y_delta){
    int left_state = 0;
    int middle_state = 0;
    int right_state = 0;
    // left button
    if (buttons & 0x01){
        left_state = 1;
    }
    else{
        left_state = 0;
    }
    // middle button
    if (buttons & 0x04){
        middle_state = 1;
    }
    else{
        middle_state = 0;
    }
    // right buttin
    if (buttons & 0x02){
        right_state = 1;
    }
    else{
        right_state = 0;
    }
    Debug::printf("PS/2 raw: buttons=%02x (left=%d,middle=%d,right=%d), dx=%d, dy=%d\n",
                  buttons, left_state, middle_state, right_state, x_delta, y_delta);
	 //scale the position changes with mouses current sensitivity
    int scaled_x_delta = (x_delta * mouse_sensitivity) / 10;
    int scaled_y_delta = (y_delta * mouse_sensitivity) / 10;
	//multiply it by the mouses dpi factor
    int dpi_factor = mouse_dpi / 800; //preset to 800
    scaled_x_delta *= dpi_factor;
    scaled_y_delta *= dpi_factor;
    // Update mouse position
    mouse_x += scaled_x_delta;
    mouse_y += scaled_y_delta;
    //position changed?
    if (x_delta != 0 || y_delta != 0){
		uint32_t sdl_event_type = SDL_MOUSEMOTION;
		uint8_t sdl_button_state = 0;
		//convert to sdl
		if (buttons & 0x01){ //left
			sdl_button_state = sdl_button_state + 1; 
		}
		if (buttons & 0x04){ //middle
			sdl_button_state = sdl_button_state + 2;  
		}
		if (buttons & 0x02){ //right
			sdl_button_state = sdl_button_state + 4; 
		}
		Debug::printf("SDL MOUSEMOTION: type=%d, state=%d, x=%d, y=%d, xrel=%d, yrel=%d\n",
							sdl_event_type, sdl_button_state, mouse_x, mouse_y, x_delta, y_delta);
    }
    uint8_t changed_buttons = buttons ^ last_buttons;

    for (int i = 0; i < 3; i++){
        uint8_t ps2_mask = 0;
        uint8_t sdl_button = 0;
        switch (i){
        case 0: 
            ps2_mask = 0x01;
            sdl_button = SDL_BUTTON_LEFT;
            break;
        case 1:
            ps2_mask = 0x02;
            sdl_button = SDL_BUTTON_RIGHT;
            break;
        case 2:
            ps2_mask = 0x04;
            sdl_button = SDL_BUTTON_MIDDLE;
            break;
        }

        if (changed_buttons & ps2_mask){
            bool is_pressed = buttons & ps2_mask;
            uint32_t sdl_event_type = is_pressed ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
            uint8_t sdl_button_state = is_pressed ? 1 : 0;
            Debug::printf("SDL %s: type=%d, button=%d, state=%d, x=%d, y=%d\n", is_pressed ? "MOUSEBUTTONDOWN" : "MOUSEBUTTONUP",
                          sdl_event_type, sdl_button, sdl_button_state, mouse_x, mouse_y);
            Debug::printf("Mouse button %d %s at x=%d, y=%d\n", sdl_button, is_pressed ? "pressed" : "released", mouse_x, mouse_y);
        }
    }
    Debug::printf("left: %d, middle: %d, right: %d, x: %d, y:%d\n", buttons & 0x1, (buttons & 0x4) >> 2, (buttons & 0x2) >> 1, mouse_x, mouse_y);
    last_buttons = buttons;
    buff.enqueue(buttons);
    buff.enqueue(mouse_x);
    buff.enqueue(mouse_y);
}
