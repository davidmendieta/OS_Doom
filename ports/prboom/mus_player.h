/* Stock Doom MUS playback, using the IWAD's GENMIDI instruments. */
#ifndef OS_MUS_PLAYER_H
#define OS_MUS_PLAYER_H
#include <stddef.h>
#include <stdint.h>
#define MUS_RATE 48000
int mus_init(const void *bank, size_t length);
int mus_register(const void *data, size_t length);
void mus_unregister(void);
void mus_play(int loop);
void mus_stop(void);
void mus_pause(int pause);
void mus_volume(int volume); /* 0..15 */
int mus_active(void);
void mus_render(int16_t *stereo, unsigned frames);
/* Read-only playback position, also used by the host regression test. */
uint64_t mus_position(void);
unsigned mus_loops(void);
#endif
