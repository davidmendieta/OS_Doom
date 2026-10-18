#ifndef OS_INPUT_H
#define OS_INPUT_H
#define OS_SYS_INPUT_READ 115
#define OS_INPUT_KEY_COUNT 512
#define OS_INPUT_EVENT_COUNT 32
#define OS_INPUT_MOUSE_LEFT 512
#define OS_INPUT_MOUSE_RIGHT 513
#define OS_INPUT_MOUSE_MIDDLE 514
#define OS_INPUT_PAUSE 511
#ifndef __ASSEMBLER__
#include <stdint.h>
typedef struct { uint16_t code, down; } os_input_event;
/* Set-2 key codes, with 0x100 added for E0 extended keys. Mouse bits are
 * left=1, right=2, middle=4. Motion is relative, positive right/down.
 * One reader owns this global input stream. read never waits for an event.
 * On overflow, events are discarded; keys/buttons still give current state.
 */
typedef struct {
    uint32_t keys[OS_INPUT_KEY_COUNT / 32];
    uint32_t buttons;
    int32_t dx, dy;
    uint32_t count, overflow;
    os_input_event events[OS_INPUT_EVENT_COUNT];
} os_input_frame;
#ifdef __cplusplus
extern "C" {
#endif
int os_input_read(os_input_frame *frame);
#ifdef __cplusplus
}
#endif
#endif
#endif
