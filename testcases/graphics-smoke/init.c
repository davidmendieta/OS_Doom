#include "os/video.h"

extern int write(unsigned fd, const void *buffer, unsigned size);
extern int open(const char *path, int flags, unsigned mode);
extern int close(unsigned fd);

#define SAY(text) write(1, text, sizeof(text) - 1)
#define REQUIRE(condition, message) do { \
    if (!(condition)) { SAY("*** graphics: FAIL " message "\n"); return 1; } \
} while (0)

static uint8_t frame[OS_VIDEO_FRAME_BYTES];
static os_video_color palette[OS_VIDEO_COLORS];

int main(void) {
    REQUIRE(os_video_init() == 0, "mode initialization");
    int fd = open("/dev/vga", 0, 0);
    REQUIRE(fd >= 0, "open /dev/vga");

    REQUIRE(os_video_set_palette(palette, 256, 1) == -1, "palette start bound");
    REQUIRE(os_video_set_palette(palette, 255, 2) == -1, "palette end bound");
    REQUIRE(os_video_set_palette(palette, 0, ~0u) == -1, "palette count overflow");
    REQUIRE(os_video_set_palette(palette, 0, 0) == -1, "empty palette");
    REQUIRE(os_video_set_palette((void*)0, 0, 1) == -1, "null palette");
    REQUIRE(os_video_set_palette((void*)0xA0000, 0, 1) == -1, "kernel palette pointer");
    REQUIRE(os_video_set_palette((void*)0xE0000000, 0, 1) == -1, "unmapped palette");
    REQUIRE(os_video_set_palette((void*)0xEFFFFFFF, 0, 1) == -1, "palette crossing user limit");
    REQUIRE(write(fd, (void*)0xE0000000, sizeof(frame)) == -1, "unmapped frame");
    REQUIRE(write(fd, frame, ~0u) == -1, "negative frame length");

    /* All 256 indices: red varies by column, green by row, blue alternates.
     * Multiples of four make the expected DAC quantization unambiguous. */
    for (unsigned i = 0; i < OS_VIDEO_COLORS; ++i) {
        palette[i].r = (i & 15) * 16;
        palette[i].g = (i >> 4) * 16;
        palette[i].b = (i & 1) ? 252 : 0;
    }
    REQUIRE(os_video_set_palette(palette, 0, OS_VIDEO_COLORS) == 0, "full palette");
    /* Verify a nonzero first index, used by partial palette updates. */
    const os_video_color white = {252, 252, 252};
    REQUIRE(os_video_set_palette(&white, 255, 1) == 0, "partial palette");

    for (unsigned y = 0; y < OS_VIDEO_HEIGHT; ++y) {
        for (unsigned x = 0; x < OS_VIDEO_WIDTH; ++x) {
            frame[y * OS_VIDEO_WIDTH + x] = (y * 16 / OS_VIDEO_HEIGHT) * 16 + x / 20;
        }
    }
    /* The asymmetric border and diagonals expose row/pitch/orientation errors. */
    for (unsigned y = 0; y < OS_VIDEO_HEIGHT; ++y) {
        frame[y * OS_VIDEO_WIDTH] = 255;
        frame[y * OS_VIDEO_WIDTH + (y * 3 / 2)] = 255;
    }
    REQUIRE(write(fd, frame, sizeof(frame)) == sizeof(frame), "frame write");
    REQUIRE(close(fd) == 0, "close /dev/vga");
    SAY("*** graphics: READY frame and palette\n");
    /* Keep the frame visible for a human or the QMP screenshot verifier. */
    for (;;) __asm__ volatile("pause");
}
