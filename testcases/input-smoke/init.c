#include <assert.h>
#include <stdio.h>
#include <os/input.h>
#include <os/runtime.h>

static void delay(unsigned ms) {
    unsigned start = os_milliseconds();
    while ((unsigned)(os_milliseconds() - start) < ms) os_yield();
}
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    os_input_frame frame;
    assert(os_input_read(NULL) == -1);
    assert(os_input_read((void*)0xa0000) == -1);
    assert(os_input_read((void*)0xe0000000) == -1);
    assert(os_input_read((void*)0xefffffff) == -1);
    assert(os_input_read(&frame) == 0);
    puts("*** input: flood ready");
    // Deliberately leave input unread while QEMU sends more than 32 events.
    delay(3500);
    assert(os_input_read(&frame) == 0 && frame.overflow && frame.count == 0);
    for (unsigned i = 0; i < 16; ++i)
        assert(frame.keys[i] == (i == 0x175 / 32 ? 1u << (0x175 % 32) : 0));
    assert(frame.buttons == 1 && frame.dx == 12 && frame.dy == -7);
    puts("*** input: release ready");
    delay(1500);
    assert(os_input_read(&frame) == 0 && !frame.overflow);
    for (unsigned i = 0; i < 16; ++i) assert(frame.keys[i] == 0);
    assert(frame.buttons == 0 && frame.dx == 0 && frame.dy == 0);
    int enter_down = 0, enter_up = 0;
    for (unsigned i = 0; i < frame.count; ++i) {
        if (frame.events[i].code != 0x5a) continue;
        if (frame.events[i].down) enter_down = 1;
        else { assert(enter_down); enter_up = 1; }
    }
    assert(enter_down && enter_up);
    assert(os_input_read(&frame) == 0 && frame.count == 0 && !frame.overflow);
    puts("*** input: PASS bounds, overflow, held state, releases, quick tap, motion");
    return 0;
}
