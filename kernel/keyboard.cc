#include "debug.h"
#include "idt.h"
#include "machine.h"
#include "keyboard.h"
#include "ps2.h"
#include "sdl_keycodes.h"
#include "sys.h"
#include "pit.h"
#include "ext2.h"

#include "libc.h"

#include "kernel.h"

//#include "d_event.h"
//#include "doomkeys.h"

#define SDL_KEYDOWN 0x300
#define SDL_KEYUP   0x301


RingBuffer<int16_t> event_buffer(100); 


uint8_t buffer[3]; // buffer for keyboard input print testing


char input_buffer[100];

// to be called in read() syscall to handler sdl events
// would read from the SDL event file descriptor
bool ready_to_read(uint8_t* event){
    if (event_buffer.isEmpty()) {
        return false;
    }
    *event = event_buffer.peek();
    return true;
}


void write_key_to_dev(uint8_t scancode, bool is_release) {
    if (is_release) return;
    
    char ascii_val = PS2::scancode_to_ascii(scancode);

    char kernel_file_name[] = "/dev/keyboard.txt";
    auto node = fs->find(fs->root, kernel_file_name);

    if (node == nullptr) {
        node = fs->create_inode(fs->root, kernel_file_name, 0x81A4);
        if (node == nullptr) {
            Debug::printf("failed to create keyboard.txt file\n");
            return;
        }
    }

    StrongPtr<OpenFile> file = StrongPtr<OpenFile>::make(node);
    file->offset = node->size_in_bytes();
    
    uint32_t written = node->write_all(file->offset, 1, &ascii_val);

    if (written == 0) {
        Debug::printf("Write failed\n");
    } else {
            node->data.size_low = file->offset + 1;
            fs->buffer_cache->write_all(node->get_offset(), sizeof(NodeData), (char*)&node->data);
            Debug::printf("--Wrote char: %c to file\n", ascii_val);
    }
}


// Converts SDL scancode to string representation for debugging purposes
const char* sdlScancodeToString(SDL_Scancode scancode) {
    switch (scancode) {
        // letters
        case SDL_SCANCODE_A: return "SDL_SCANCODE_A";
        case SDL_SCANCODE_B: return "SDL_SCANCODE_B";
        case SDL_SCANCODE_C: return "SDL_SCANCODE_C";
        case SDL_SCANCODE_D: return "SDL_SCANCODE_D";
        case SDL_SCANCODE_E: return "SDL_SCANCODE_E";
        case SDL_SCANCODE_F: return "SDL_SCANCODE_F";
        case SDL_SCANCODE_G: return "SDL_SCANCODE_G";
        case SDL_SCANCODE_H: return "SDL_SCANCODE_H";
        case SDL_SCANCODE_I: return "SDL_SCANCODE_I";
        case SDL_SCANCODE_J: return "SDL_SCANCODE_J";
        case SDL_SCANCODE_K: return "SDL_SCANCODE_K";
        case SDL_SCANCODE_L: return "SDL_SCANCODE_L";
        case SDL_SCANCODE_M: return "SDL_SCANCODE_M";
        case SDL_SCANCODE_N: return "SDL_SCANCODE_N";
        case SDL_SCANCODE_O: return "SDL_SCANCODE_O";
        case SDL_SCANCODE_P: return "SDL_SCANCODE_P";
        case SDL_SCANCODE_Q: return "SDL_SCANCODE_Q";
        case SDL_SCANCODE_R: return "SDL_SCANCODE_R";
        case SDL_SCANCODE_S: return "SDL_SCANCODE_S";
        case SDL_SCANCODE_T: return "SDL_SCANCODE_T";
        case SDL_SCANCODE_U: return "SDL_SCANCODE_U";
        case SDL_SCANCODE_V: return "SDL_SCANCODE_V";
        case SDL_SCANCODE_W: return "SDL_SCANCODE_W";
        case SDL_SCANCODE_X: return "SDL_SCANCODE_X";
        case SDL_SCANCODE_Y: return "SDL_SCANCODE_Y";
        case SDL_SCANCODE_Z: return "SDL_SCANCODE_Z";
       
        // numbers
        case SDL_SCANCODE_1: return "SDL_SCANCODE_1";
        case SDL_SCANCODE_2: return "SDL_SCANCODE_2";
        case SDL_SCANCODE_3: return "SDL_SCANCODE_3";
        case SDL_SCANCODE_4: return "SDL_SCANCODE_4";
        case SDL_SCANCODE_5: return "SDL_SCANCODE_5";
        case SDL_SCANCODE_6: return "SDL_SCANCODE_6";
        case SDL_SCANCODE_7: return "SDL_SCANCODE_7";
        case SDL_SCANCODE_8: return "SDL_SCANCODE_8";
        case SDL_SCANCODE_9: return "SDL_SCANCODE_9";
        case SDL_SCANCODE_0: return "SDL_SCANCODE_0";
       
        // special keys
        case SDL_SCANCODE_RETURN: return "SDL_SCANCODE_RETURN";
        case SDL_SCANCODE_ESCAPE: return "SDL_SCANCODE_ESCAPE";
        case SDL_SCANCODE_BACKSPACE: return "SDL_SCANCODE_BACKSPACE";
        case SDL_SCANCODE_TAB: return "SDL_SCANCODE_TAB";
        case SDL_SCANCODE_SPACE: return "SDL_SCANCODE_SPACE";
        case SDL_SCANCODE_MINUS: return "SDL_SCANCODE_MINUS";
        case SDL_SCANCODE_EQUALS: return "SDL_SCANCODE_EQUALS";
        case SDL_SCANCODE_LEFTBRACKET: return "SDL_SCANCODE_LEFTBRACKET";
        case SDL_SCANCODE_RIGHTBRACKET: return "SDL_SCANCODE_RIGHTBRACKET";
        case SDL_SCANCODE_BACKSLASH: return "SDL_SCANCODE_BACKSLASH";
        case SDL_SCANCODE_SEMICOLON: return "SDL_SCANCODE_SEMICOLON";
        case SDL_SCANCODE_APOSTROPHE: return "SDL_SCANCODE_APOSTROPHE";
        case SDL_SCANCODE_GRAVE: return "SDL_SCANCODE_GRAVE";
        case SDL_SCANCODE_COMMA: return "SDL_SCANCODE_COMMA";
        case SDL_SCANCODE_PERIOD: return "SDL_SCANCODE_PERIOD";
        case SDL_SCANCODE_SLASH: return "SDL_SCANCODE_SLASH";
        case SDL_SCANCODE_RSHIFT: return "SDL_SCANCODE_RSHIFT";
       
        // Modifier keys
        case SDL_SCANCODE_LCTRL: return "SDL_SCANCODE_LCTRL";
        case SDL_SCANCODE_LSHIFT: return "SDL_SCANCODE_LSHIFT";
        case SDL_SCANCODE_LALT: return "SDL_SCANCODE_LALT";
       
        // arrow keys
        case SDL_SCANCODE_UP: return "SDL_SCANCODE_UP";
        case SDL_SCANCODE_DOWN: return "SDL_SCANCODE_DOWN";
        case SDL_SCANCODE_LEFT: return "SDL_SCANCODE_LEFT";
        case SDL_SCANCODE_RIGHT: return "SDL_SCANCODE_RIGHT";
       
        // default case
        case SDL_SCANCODE_UNKNOWN: return "SDL_SCANCODE_UNKNOWN";
        default: return "UNKNOWN_SCANCODE";
    }
}

void keyboardHandler(uint8_t scancode) {
   
    for(uint32_t i=0; i< 3; i++){
        if(buffer[0] == 0){
            buffer[0] = scancode;
            break;
        }
        if(buffer[1] == 0){
            buffer[1] = scancode;
            break;
        }
        if(buffer[2] == 0){
            buffer[2] = scancode;
            break;
        }
        buffer[0] = 0;
        buffer[1] = 0;
        buffer[2] = 0;
    }


    bool is_release = scancode & 0x80;
    uint8_t keycode = scancode & 0x7F;

    static bool is_break = false;


    SDL_Scancode sdl_scancode;
   
    switch (keycode) {
        // letters
        case 0x1C: sdl_scancode = SDL_SCANCODE_A; break;
        case 0x32: sdl_scancode = SDL_SCANCODE_B; break;
        case 0x21: sdl_scancode = SDL_SCANCODE_C; break;
        case 0x23: sdl_scancode = SDL_SCANCODE_D; break;
        case 0x24: sdl_scancode = SDL_SCANCODE_E; break;
        case 0x2B: sdl_scancode = SDL_SCANCODE_F; break;
        case 0x34: sdl_scancode = SDL_SCANCODE_G; break;
        case 0x33: sdl_scancode = SDL_SCANCODE_H; break;
        case 0x43: sdl_scancode = SDL_SCANCODE_I; break;
        case 0x3B: sdl_scancode = SDL_SCANCODE_J; break;
        case 0x42: sdl_scancode = SDL_SCANCODE_K; break;
        case 0x4B: sdl_scancode = SDL_SCANCODE_L; break;
        case 0x3A: sdl_scancode = SDL_SCANCODE_M; break;
        case 0x31: sdl_scancode = SDL_SCANCODE_N; break;
        case 0x44: sdl_scancode = SDL_SCANCODE_O; break;
        case 0x4D: sdl_scancode = SDL_SCANCODE_P; break;
        case 0x15: sdl_scancode = SDL_SCANCODE_Q; break;
        case 0x2D: sdl_scancode = SDL_SCANCODE_R; break;
        case 0x1B: sdl_scancode = SDL_SCANCODE_S; break;
        case 0x2C: sdl_scancode = SDL_SCANCODE_T; break;
        case 0x3C: sdl_scancode = SDL_SCANCODE_U; break;
        case 0x2A: sdl_scancode = SDL_SCANCODE_V; break;
        case 0x1D: sdl_scancode = SDL_SCANCODE_W; break;
        case 0x22: sdl_scancode = SDL_SCANCODE_X; break;
        case 0x35: sdl_scancode = SDL_SCANCODE_Y; break;
        case 0x1A: sdl_scancode = SDL_SCANCODE_Z; break;
       
        // number
        case 0x16: sdl_scancode = SDL_SCANCODE_1; break;
        case 0x1E: sdl_scancode = SDL_SCANCODE_2; break;
        case 0x26: sdl_scancode = SDL_SCANCODE_3; break;
        case 0x25: sdl_scancode = SDL_SCANCODE_4; break;
        case 0x2E: sdl_scancode = SDL_SCANCODE_5; break;
        case 0x36: sdl_scancode = SDL_SCANCODE_6; break;
        case 0x3D: sdl_scancode = SDL_SCANCODE_7; break;
        case 0x3E: sdl_scancode = SDL_SCANCODE_8; break;
        case 0x46: sdl_scancode = SDL_SCANCODE_9; break;
        case 0x45: sdl_scancode = SDL_SCANCODE_0; break;
       
        // special keys
        case 0x5A: sdl_scancode = SDL_SCANCODE_RETURN; break; // enter
        case 0x76: sdl_scancode = SDL_SCANCODE_ESCAPE; break; // escape
        case 0x66: sdl_scancode = SDL_SCANCODE_BACKSPACE; break; // backspace
        case 0x0D: sdl_scancode = SDL_SCANCODE_TAB; break; // tab
        case 0x29: sdl_scancode = SDL_SCANCODE_SPACE; break; // space
        // minus
        // equals
        // left
        // right
        // backslash
        // semicolon
        // apostrophe
        // grave

        case 0x41: sdl_scancode = SDL_SCANCODE_COMMA; break; // comma
        case 0x49: sdl_scancode = SDL_SCANCODE_PERIOD; break; // period
        // slash
        case 0x59: sdl_scancode = SDL_SCANCODE_RSHIFT; break; // right shift
   
        // modifer keys
        case 0x14: sdl_scancode = SDL_SCANCODE_LCTRL; break; // left control
        case 0x12: sdl_scancode = SDL_SCANCODE_LSHIFT; break; // left shift
        case 0x11: sdl_scancode = SDL_SCANCODE_LALT; break; // alt
       
        // arrow keys
        // default case
        default: sdl_scancode = SDL_SCANCODE_UNKNOWN; break;
    }


    // determine the ascii value
    char ascii_val = PS2::scancode_to_ascii(scancode);
    int sym = (int) ascii_val;


    // determine modifier keys
    uint16_t mod = 0;
    if (keycode == 0x2A || keycode == 0x36) mod |= 1; // shift
    if (keycode == 0x1D) mod |= 2; // ctrl


    // create the event with all fields
    SDL_KeyEvent event = {
        uint32_t(is_release ? SDL_KEYUP : SDL_KEYDOWN),  // type
        0,                                      // timestamp
        {                                       // keysym
            sdl_scancode,                       // scancode
            sym,                                // sym
            mod,                                // mod
        }
    };    
    event.timestamp = Pit::jiffies; 
    (void) event; // silence
    event_buffer.enqueue(scancode); // add to buffer to be used by poll()


    bool second = false;



    if(buffer[0] == buffer[1] && buffer[2] == 0xF0){
        // this is a break code
        // we need to set the break flag
        is_break = true;
    } else if(buffer[0]!= buffer[1]){
        // this is a make code
        // we need to clear the break flag
        is_break = false;
    }else{
        second = true;
    }
    if(!second){
    if(!is_break){
        write_key_to_dev(scancode, is_release);
    // Debug::printf("Key [%s]: scancode=%x, ascii=%c, sdl_code=%s\n",
    //             "pressed",
    //             scancode,
    //             (sym > 0 && sym < 128) ? sym : '?', sdlScancodeToString(sdl_scancode));    
    } else {
        // Debug::printf("Key [%s]: scancode=%x%x, ascii=%c, sdl_code=%s\n",
        //     "released",
        //     scancode,buffer[1],
        //     (sym > 0 && sym < 128) ? sym : '?', sdlScancodeToString(sdl_scancode));    
    }
}


    // THIS WAS OLD VERSTION DIRECTLY CALLING POST EVENT IN DOOM
    // NOW USING THE POLL SYSCALL TO SDLPOLL EVENTS

    // int doom_key = get_doom_key(sym);

    // // convert sdl to event_t for doom
    // event_t doom_event;
    // doom_event.type = (is_release ? ev_keyup : ev_keydown);
    // doom_event.data1 = doom_key;
    // doom_event.data2 = sym;

    // D_PostEvent(&doom_event);   // pass the event to Doom
}

// int get_doom_key(int sym){
//     if (sym >= 'a' && sym <= 'z') {
//         return  sym;  
//     }
//     if (sym >= '0' && sym <= '9') {
//         return sym;
//     }
   
//     //special keys
//     switch(sym) {
//         case 27: return KEY_ESCAPE;
//         case '\r': return KEY_ENTER;
//         case '\t': return KEY_TAB;
//         case '\b': return KEY_BACKSPACE;
//         case ' ': return ' ';
//         case '-': return KEY_MINUS;
//         case '=': return KEY_EQUALS;
//         case '[': return '[';
//         case ']': return ']';
//         case '\\': return '\\';
//         case ';': return ';';
//         case '\'': return '\'';
//         case '`': return '`';
//         case ',': return ',';
//         case '.': return '.';
//         case '/': return '/';
//     }

//     return 0;  // Unknown key
// }














