#include "libc.h"

#include "sys.h"

#define SCREENWIDTH  320
#define SCREENHEIGHT 200

typedef unsigned char  u8;
typedef unsigned short u16;
typedef int            bool;
#define true 1
#define false 0

typedef struct {
    int width;
    int height;
    u8* data;
    int byte_pitch;
    int short_pitch;
    int int_pitch;
    bool not_on_heap;
} viddef_t;

// Declare the global screens array
viddef_t screens[5];

void init_test_screens() {
    static u8 framebuffer[320 * 200];  // or malloc() in a real app

    screens[0].width = 320;
    screens[0].height = 200;
    screens[0].data = framebuffer;
    screens[0].byte_pitch = 320;
    screens[0].short_pitch = 160;
    screens[0].int_pitch = 80;
    screens[0].not_on_heap = true;
}

void fill_gradient() {
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 320; ++x) {
            screens[0].data[y * 320 + x] = x;  // Horizontal gradient
        }
    }
}

int run_test() {
    init_test_screens();
    int vga_fd = open("/dev/vga", 0); // your OS handles this
    screens[0].data = malloc(SCREENWIDTH * SCREENHEIGHT);  // software framebuffer

    fill_gradient();

    write(vga_fd, screens[0].data, SCREENWIDTH * SCREENHEIGHT);
    return 0;
}

int main(int argc, char** argv) {
    run_test();
    shutdown();
    return 0;
}
