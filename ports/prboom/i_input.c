/* Native PS/2 input -> PrBoom events. See include/os/input.h for the ABI. */
#include <os/input.h>
#include <stdio.h>
#include "doomstat.h"
#include "d_event.h"
#include "d_main.h"
#include "lprintf.h"

extern int usemouse;
extern int os_input_testing;
static unsigned char held[OS_INPUT_KEY_COUNT];
static unsigned references[512];
static unsigned buttons;
static const int keymap[OS_INPUT_KEY_COUNT] = {
    [0x1c]='a', [0x32]='b', [0x21]='c', [0x23]='d', [0x24]='e',
    [0x2b]='f', [0x34]='g', [0x33]='h', [0x43]='i', [0x3b]='j',
    [0x42]='k', [0x4b]='l', [0x3a]='m', [0x31]='n', [0x44]='o',
    [0x4d]='p', [0x15]='q', [0x2d]='r', [0x1b]='s', [0x2c]='t',
    [0x3c]='u', [0x2a]='v', [0x1d]='w', [0x22]='x', [0x35]='y', [0x1a]='z',
    [0x16]='1', [0x1e]='2', [0x26]='3', [0x25]='4', [0x2e]='5',
    [0x36]='6', [0x3d]='7', [0x3e]='8', [0x46]='9', [0x45]='0',
    [0x5a]=KEYD_ENTER, [0x76]=KEYD_ESCAPE, [0x66]=KEYD_BACKSPACE,
    [0x0d]=KEYD_TAB, [0x29]=' ', [0x4e]='-', [0x55]='=', [0x54]='[',
    [0x5b]=']', [0x5d]='\\', [0x4c]=';', [0x52]='\'', [0x0e]='`',
    [0x41]=',', [0x49]='.', [0x4a]='/',
    [0x12]=KEYD_RSHIFT, [0x59]=KEYD_RSHIFT, [0x14]=KEYD_RCTRL,
    [0x114]=KEYD_RCTRL, [0x11]=KEYD_RALT, [0x111]=KEYD_RALT,
    [0x58]=KEYD_CAPSLOCK, [0x77]=KEYD_NUMLOCK, [0x7e]=KEYD_SCROLLLOCK,
    [0x175]=KEYD_UPARROW, [0x172]=KEYD_DOWNARROW,
    [0x16b]=KEYD_LEFTARROW, [0x174]=KEYD_RIGHTARROW,
    [0x170]=KEYD_INSERT, [0x171]=KEYD_DEL, [0x16c]=KEYD_HOME,
    [0x169]=KEYD_END, [0x17d]=KEYD_PAGEUP, [0x17a]=KEYD_PAGEDOWN,
    [0x05]=KEYD_F1, [0x06]=KEYD_F2, [0x04]=KEYD_F3, [0x0c]=KEYD_F4,
    [0x03]=KEYD_F5, [0x0b]=KEYD_F6, [0x83]=KEYD_F7, [0x0a]=KEYD_F8,
    [0x01]=KEYD_F9, [0x09]=KEYD_F10, [0x78]=KEYD_F11, [0x07]=KEYD_F12,
    [0x70]=KEYD_KEYPAD0, [0x69]=KEYD_KEYPAD1, [0x72]=KEYD_KEYPAD2,
    [0x7a]=KEYD_KEYPAD3, [0x6b]=KEYD_KEYPAD4, [0x73]=KEYD_KEYPAD5,
    [0x74]=KEYD_KEYPAD6, [0x6c]=KEYD_KEYPAD7, [0x75]=KEYD_KEYPAD8,
    [0x7d]=KEYD_KEYPAD9, [0x71]=KEYD_KEYPADPERIOD, [0x79]=KEYD_KEYPADPLUS,
    [0x7b]=KEYD_KEYPADMINUS, [0x7c]=KEYD_KEYPADMULTIPLY,
    [0x15a]=KEYD_KEYPADENTER, [0x14a]=KEYD_KEYPADDIVIDE,
    [OS_INPUT_PAUSE]=KEYD_PAUSE
};

static void mouse_event(int dx, int dy) {
    if (!usemouse) return;
    event_t event = {ev_mouse,
        (buttons & 1) | ((buttons & 4) >> 1) | ((buttons & 2) << 1), dx, dy};
    D_PostEvent(&event);
}
static void key_event(unsigned code, int down) {
    if (code >= OS_INPUT_MOUSE_LEFT && code <= OS_INPUT_MOUSE_MIDDLE) {
        unsigned mask = 1u << (code - OS_INPUT_MOUSE_LEFT);
        buttons = down ? buttons | mask : buttons & ~mask;
        mouse_event(0, 0);
        return;
    }
    if (code >= OS_INPUT_KEY_COUNT) return;
    int key = keymap[code];
    int was = held[code];
    held[code] = down;
    if (!key) return;
    if (down && !was) ++references[key];
    if (!down && was) --references[key];
    // Both shifts/controls/alts share a Doom key: releasing one must not
    // release the other. Repeated make codes still drive menu key repeat.
    if ((!down && (!was || references[key])) || (down && !was && references[key] > 1)) return;
    event_t event = {down ? ev_keydown : ev_keyup, key, 0, 0};
    D_PostEvent(&event);
}
extern void I_PumpSound(void);
void I_StartTic(void) {
    I_PumpSound();
    os_input_frame frame;
    if (os_input_read(&frame) < 0) I_Error("Input read failed");
    for (unsigned i = 0; i < frame.count; ++i)
        key_event(frame.events[i].code, frame.events[i].down);
    // Recover from overflow without leaving movement/fire stuck down.
    for (unsigned code = 0; code < OS_INPUT_KEY_COUNT; ++code) {
        int down = (frame.keys[code / 32] >> (code % 32)) & 1;
        if (down != held[code]) key_event(code, down);
    }
    if (buttons != frame.buttons) { buttons = frame.buttons; mouse_event(0, 0); }
    if (gamestate == GS_LEVEL && !demoplayback && !paused && !menuactive && (frame.dx || frame.dy))
        mouse_event(frame.dx * 32, -frame.dy * 32);
}

/* Test-only observations of the real engine, never used to alter game state. */
void I_InputReport(void) {
    static int last_tic = -1;
    if (!os_input_testing || last_tic == gametic) return;
    last_tic = gametic;
    if (netgame && gamestate == GS_LEVEL) {
        for (int i = 0; i < 2; ++i) {
            player_t *p = &players[i];
            if (p->mo) printf("*** netstate: tic=%d player=%d x=%d y=%d angle=%u health=%d ammo=%d forward=%d buttons=%d\n",
                gametic, i, p->mo->x, p->mo->y, p->mo->angle, p->health,
                p->ammo[am_clip], p->cmd.forwardmove, p->cmd.buttons);
        }
    }
    player_t *p = &players[consoleplayer];
    printf("*** control: tic=%d menu=%d demo=%d level=%d paused=%d x=%d y=%d angle=%u ammo=%d forward=%d side=%d turn=%d buttons=%d\n",
        gametic, menuactive, demoplayback, gamestate == GS_LEVEL, paused,
        p->mo ? p->mo->x : 0, p->mo ? p->mo->y : 0, p->mo ? p->mo->angle : 0,
        p->ammo[am_clip], p->cmd.forwardmove, p->cmd.sidemove, p->cmd.angleturn, p->cmd.buttons);
}
