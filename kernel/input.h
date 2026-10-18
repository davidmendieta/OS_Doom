#pragma once
#include "../include/os/input.h"
namespace Input {
    void key(uint16_t code, bool down);
    void mouse(uint8_t buttons, int dx, int dy);
    void read(os_input_frame& out);
}
