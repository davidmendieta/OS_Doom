#ifndef OS_AUDIO_H
#define OS_AUDIO_H
#define OS_SYS_AUDIO_OPEN 116
#define OS_SYS_AUDIO_WRITE 117
#define OS_SYS_AUDIO_QUEUED 118
#define OS_SYS_AUDIO_CLOSE 119
#define OS_AUDIO_RATE 48000
#define OS_AUDIO_CHANNELS 2
#define OS_AUDIO_MAX_FRAMES 1024
#ifndef __ASSEMBLER__
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* One foreground PCM producer. Signed 16-bit little-endian interleaved stereo.
 * Counts are stereo FRAMES, not bytes or individual samples. Nonblocking write
 * returns frames accepted, 0 for a full queue, -1 for invalid/unavailable.
 * Open/close discard queued audio. queued() reports DMA frames remaining.
 */
int os_audio_open(void);
int os_audio_write(const int16_t *pcm, unsigned frames);
int os_audio_queued(void);
int os_audio_close(void);
#ifdef __cplusplus
}
#endif
#endif
#endif
