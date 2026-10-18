/* Explicitly silent backend for the first video/runtime milestone. */
#include <stddef.h>
#include "i_sound.h"
int snd_card, mus_card, detect_voices, snd_samplerate = 11025;
void I_InitSound(void) {}
void I_ShutdownSound(void) {}
void I_SetChannels(void) {}
int I_GetSfxLumpNum(sfxinfo_t *sfx) { (void)sfx; return -1; }
int I_StartSound(int id, int ch, int vol, int sep, int pitch, int priority) { return -1; }
void I_StopSound(int handle) {}
boolean I_SoundIsPlaying(int handle) { return false; }
boolean I_AnySoundStillPlaying(void) { return false; }
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch) {}
void I_InitMusic(void) {}
void I_ShutdownMusic(void) {}
void I_UpdateMusic(void) {}
void I_SetMusicVolume(int volume) {}
void I_PauseSong(int handle) {}
void I_ResumeSong(int handle) {}
int I_RegisterSong(const void *data, size_t len) { return -1; }
int I_RegisterMusic(const char *filename, musicinfo_t *music) { return -1; }
void I_PlaySong(int handle, int looping) {}
void I_StopSong(int handle) {}
void I_UnRegisterSong(int handle) {}
