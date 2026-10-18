#include "input.h"
#include "atomic.h"

namespace {
SpinLock lock;
os_input_frame pending{};
void enqueue(uint16_t code, bool down) {
    if (pending.count == OS_INPUT_EVENT_COUNT) pending.overflow = 1;
    if (!pending.overflow) pending.events[pending.count++] = {code, uint16_t(down)};
}
int accumulate(int previous, int delta) {
    int next = previous + delta;
    return next < -16384 ? -16384 : next > 16384 ? 16384 : next;
}
}

void Input::key(uint16_t code, bool down) {
    if (code >= OS_INPUT_KEY_COUNT) return;
    LockGuard guard{lock};
    uint32_t mask = uint32_t(1) << (code % 32);
    if (down) pending.keys[code / 32] |= mask;
    else pending.keys[code / 32] &= ~mask;
    enqueue(code, down);
}
void Input::mouse(uint8_t buttons, int dx, int dy) {
    LockGuard guard{lock};
    for (unsigned bit = 0; bit < 3; ++bit) {
        if ((pending.buttons ^ buttons) & (1u << bit))
            enqueue(OS_INPUT_MOUSE_LEFT + bit, buttons & (1u << bit));
    }
    pending.buttons = buttons & 7;
    pending.dx = accumulate(pending.dx, dx);
    pending.dy = accumulate(pending.dy, dy);
}
void Input::read(os_input_frame& out) {
    LockGuard guard{lock};
    out = pending;
    if (out.overflow) out.count = 0;
    pending.count = pending.overflow = 0;
    pending.dx = pending.dy = 0;
}
