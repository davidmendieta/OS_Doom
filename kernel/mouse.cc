#include "mouse.h"
#include "input.h"

RingBuffer<int16_t> buff(256);
void mouse_init() {}
void mouseHandler(uint8_t buttons, int16_t dx, int16_t dy) {
    Input::mouse(buttons, dx, dy);
    // Preserve absolute 16-bit coordinates for legacy device consumers.
    static uint16_t x, y;
    x += dx;
    y += dy;
    const int16_t packet[] = {buttons, int16_t(x), int16_t(y)};
    buff.enqueue(packet, 3);
}
