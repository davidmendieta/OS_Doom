#ifndef OS_VIDEO_H
#define OS_VIDEO_H

/* Mode 13h: one palette index per pixel, rows packed without padding. */
#define OS_VIDEO_WIDTH 320
#define OS_VIDEO_HEIGHT 200
#define OS_VIDEO_FRAME_BYTES (OS_VIDEO_WIDTH * OS_VIDEO_HEIGHT)
#define OS_VIDEO_COLORS 256
#define OS_SYS_VIDEO_INIT 103
#define OS_SYS_VIDEO_PALETTE 105

#ifndef __ASSEMBLER__
#include <stdint.h>

typedef struct {
    uint8_t r, g, b;
} os_video_color;

#ifdef __cplusplus
extern "C" {
#endif

/* Implemented in userspace/video.S. Returns 0 on success, -1 on error.
 * Palette components are 8-bit RGB; VGA quantizes each to six bits.
 * colors contains count entries to install beginning at first (count > 0).
 * Submit frames with write(open("/dev/vga", 0, 0), pixels, 64000).
 */
int os_video_init(void);
int os_video_set_palette(const os_video_color *colors, unsigned first, unsigned count);

#ifdef __cplusplus
}
#endif
#endif
#endif
