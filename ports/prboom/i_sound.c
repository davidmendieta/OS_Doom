/* Native, single-threaded DMX sound-effect mixer feeding the AC'97 PCM queue. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <os/audio.h>
#include <os/runtime.h>
#include "doomstat.h"
#include "d_main.h"
#include "i_sound.h"
#include "s_sound.h"
#include "w_wad.h"
#include "mus_player.h"

#define MIX_CHANNELS 32
#define QUEUE_FRAMES (3 * OS_AUDIO_MAX_FRAMES)
int snd_card = 1, mus_card = 1, detect_voices, snd_samplerate = OS_AUDIO_RATE;
extern int os_input_testing;
static struct voice {
    const unsigned char *data;
    int lump, left, right;
    unsigned length, rate, index, fraction, step;
} voices[MIX_CHANNELS];
static unsigned pitch_steps[256];
static int16_t pcm[OS_AUDIO_MAX_FRAMES * OS_AUDIO_CHANNELS];
static int sound_ready, pending, pending_sfx;
static uint64_t submitted, sfx_end;
static uint32_t last_pump;

static int clamp(int n, int low, int high) {
    return n < low ? low : n > high ? high : n;
}
void I_StopSound(int handle) {
    if (handle < 0 || handle >= MIX_CHANNELS || !voices[handle].data) return;
    W_UnlockLumpNum(voices[handle].lump);
    voices[handle].data = NULL;
}
void I_ShutdownSound(void) {
    if (!sound_ready) return;
    sound_ready = 0;
    pending = pending_sfx = 0;
    submitted = sfx_end = 0;
    I_ShutdownMusic();
    os_audio_close();
    for (int i = 0; i < MIX_CHANNELS; ++i) I_StopSound(i);
}
void I_InitSound(void) {
    I_ShutdownSound();
    if (os_audio_open() < 0) {
        puts("*** doom: AC97 unavailable; sound effects disabled");
        return;
    }
    sound_ready = 1;
    snd_samplerate = OS_AUDIO_RATE;
    static int registered;
    if (!registered) { atexit(I_ShutdownSound); registered = 1; }
    puts("*** doom: sound effects ready (48000 Hz stereo)");
    if (!nomusicparm) I_InitMusic();
}
void I_SetChannels(void) {
    numChannels = clamp(numChannels, 1, MIX_CHANNELS);
    for (int i = 0; i < MIX_CHANNELS; ++i) I_StopSound(i);
    // Preserve PrBoom's pitch adjustment at the selected output rate.
    for (int i = 0; i < 256; ++i)
        pitch_steps[i] = pow(1.2, (i - 128) / (64.0 * OS_AUDIO_RATE / 11025)) * 65536.0;
}
int I_GetSfxLumpNum(sfxinfo_t *sfx) {
    char name[9];
    snprintf(name, sizeof(name), "ds%.6s", sfx->name);
    return W_CheckNumForName(name);
}
void I_UpdateSoundParams(int handle, int volume, int separation, int pitch) {
    if (handle < 0 || handle >= MIX_CHANNELS || !voices[handle].data) return;
    struct voice *v = &voices[handle];
    volume = clamp(volume, 0, 127);
    separation = clamp(separation, 0, 255) + 1;
    v->left = volume - ((volume * separation * separation) >> 16);
    separation -= 257;
    v->right = volume - ((volume * separation * separation) >> 16);
    int step = ((uint64_t)v->rate << 16) / OS_AUDIO_RATE;
    if (pitched_sounds) step += (int)pitch_steps[clamp(pitch, 0, 255)] - 65536;
    v->step = step > 0 ? step : 1;
}
int I_StartSound(int id, int ch, int vol, int sep, int pitch, int priority) {
    (void)priority; // Channel selection and priorities belong to S_StartSound.
    if (!sound_ready || id <= 0 || id >= NUMSFX || ch < 0 || ch >= numChannels || ch >= MIX_CHANNELS) return -1;
    int lump = S_sfx[id].lumpnum;
    if (lump < 0) return -1;
    unsigned size = W_LumpLength(lump);
    if (size <= 8) return -1;
    const unsigned char *raw = W_LockLumpNum(lump);
    unsigned rate = raw[2] | (raw[3] << 8);
    unsigned length = raw[4] | (raw[5] << 8) | (raw[6] << 16) | ((uint32_t)raw[7] << 24);
    if (raw[0] != 3 || raw[1] || !rate || !length || length > size - 8) {
        W_UnlockLumpNum(lump);
        return -1;
    }
    I_StopSound(ch);
    voices[ch] = (struct voice){.data = raw + 8, .lump = lump, .length = length, .rate = rate};
    I_UpdateSoundParams(ch, vol, sep, pitch);
    if (os_input_testing)
        printf("*** sound: %s rate=%u samples=%u\n", S_sfx[id].name, rate, length);
    return ch;
}
boolean I_SoundIsPlaying(int handle) {
    return handle >= 0 && handle < MIX_CHANNELS && voices[handle].data != NULL;
}
static int active_voices(void) {
    for (int i = 0; i < MIX_CHANNELS; ++i)
        if (voices[i].data) return 1;
    return 0;
}
static void mix(void) {
    mus_render(pcm, OS_AUDIO_MAX_FRAMES);
    for (unsigned frame = 0; frame < OS_AUDIO_MAX_FRAMES; ++frame) {
        int left = pcm[frame * 2], right = pcm[frame * 2 + 1];
        for (int ch = 0; ch < numChannels; ++ch) {
            struct voice *v = &voices[ch];
            if (!v->data) continue;
            int first = v->data[v->index] - 128;
            int next = v->index + 1 < v->length ? v->data[v->index + 1] - 128 : first;
            int sample = first * 256 + ((next - first) * (int)v->fraction) / 256;
            left += sample * v->left / 191;
            right += sample * v->right / 191;
            unsigned position = v->fraction + v->step;
            v->index += position >> 16;
            v->fraction = position & 65535;
            if (v->index >= v->length) I_StopSound(ch);
        }
        pcm[frame * 2] = clamp(left, -32768, 32767);
        pcm[frame * 2 + 1] = clamp(right, -32768, 32767);
    }
}
// Called during engine ticks, rendering and idle waits. Never block gameplay
// waiting for DMA. Keep a short queue; retain mixed PCM if a write is deferred.
void I_PumpSound(void) {
    uint32_t now = os_milliseconds();
    if (!sound_ready || (uint32_t)(now - last_pump) < 4) return;
    last_pump = now;
    int queued = os_audio_queued();
    if (queued < 0) { I_ShutdownSound(); return; }
    while (queued < QUEUE_FRAMES) {
        // Let the hardware emit silence when idle. This also lets quit wait
        // for the final effect's DMA frames without queuing silence forever.
        if (!pending && !active_voices() && !mus_active()) return;
        if (!pending) { pending_sfx = active_voices(); mix(); pending = 1; }
        int written = os_audio_write(pcm, OS_AUDIO_MAX_FRAMES);
        if (written < 0) { I_ShutdownSound(); return; }
        if (!written) return;
        submitted += written;
        if (pending_sfx) sfx_end = submitted;
        pending = pending_sfx = 0;
        queued += written;
    }
}
boolean I_AnySoundStillPlaying(void) {
    I_PumpSound();
    if (!sound_ready) return false;
    int queued = os_audio_queued();
    return active_voices() || pending_sfx || (queued >= 0 && sfx_end > submitted - (unsigned)queued);
}
