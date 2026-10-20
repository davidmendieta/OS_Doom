/* PrBoom music lifecycle adapter. The stock IWAD uses MUS, not external MIDI. */
#include <stdio.h>
#include "i_sound.h"
#include "mus_player.h"
#include "w_wad.h"
static int ready, registered, song_loop, paused_mode = -1;
extern int mus_pause_opt;
void I_InitMusic(void) {
    int lump = W_CheckNumForName("GENMIDI");
    if (lump < 0) return;
    const void *data = W_LockLumpNum(lump);
    ready = mus_init(data, W_LumpLength(lump));
    W_UnlockLumpNum(lump);
    puts(ready ? "*** doom: OPL music ready" : "*** doom: invalid GENMIDI instruments");
}
void I_ShutdownMusic(void) { mus_unregister(); ready = registered = 0; }
void I_UpdateMusic(void) {}
void I_SetMusicVolume(int volume) { mus_volume(volume); }
void I_PauseSong(int handle) {
    if (!registered || handle != 1) return;
    paused_mode = mus_pause_opt;
    if (paused_mode == 0) mus_stop();
    if (paused_mode == 1) mus_pause(1);
    // Mode 2 is PrBoom's default: music continues while the game is paused.
}
void I_ResumeSong(int handle) {
    if (!registered || handle != 1) return;
    if (paused_mode == 0) mus_play(song_loop);
    if (paused_mode == 1) mus_pause(0);
    paused_mode = -1;
}
int I_RegisterSong(const void *data, size_t len) {
    registered = ready && mus_register(data, len);
    if (!registered) puts("*** doom: unsupported or invalid music (MUS required)");
    return registered ? 1 : -1;
}
int I_RegisterMusic(const char *filename, musicinfo_t *music) {
    (void)filename; (void)music;
    return 1; // ask S_ChangeMusic to use the IWAD's MUS lump
}
void I_PlaySong(int handle, int looping) {
    if (!registered || handle != 1) return;
    song_loop = looping;
    paused_mode = -1;
    mus_play(looping);
}
void I_StopSong(int handle) {
    if (registered && handle == 1) mus_stop();
    paused_mode = -1;
}
void I_UnRegisterSong(int handle) { if (handle == 1) { mus_unregister(); registered = 0; } }
