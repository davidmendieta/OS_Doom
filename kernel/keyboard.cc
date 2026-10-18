#include "keyboard.h"
#include "input.h"

// Retained for the legacy device/poll interface. Native applications use Input.
RingBuffer<int16_t> event_buffer(100);

void keyboardHandler(uint8_t byte) {
    static bool extended = false, released = false;
    static unsigned pause_bytes = 0;
    event_buffer.enqueue(byte);
    // Pause is E1 14 77 E1 F0 14 F0 77; it has no separate break sequence.
    if (pause_bytes) {
        if (--pause_bytes == 0) {
            Input::key(OS_INPUT_PAUSE, true);
            Input::key(OS_INPUT_PAUSE, false);
        }
        return;
    }
    if (byte == 0xe1) { pause_bytes = 7; extended = released = false; return; }
    if (byte == 0xe0) { extended = true; return; }
    if (byte == 0xf0) { released = true; return; }
    if (byte == 0xfa || byte == 0xfe || byte == 0xaa) return;
    // Print Screen carries fake E0-shift bytes; they aren't real modifiers.
    if (!(extended && byte == 0x12))
        Input::key(byte | (extended ? 0x100 : 0), !released);
    extended = released = false;
}
