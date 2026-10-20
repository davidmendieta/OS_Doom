#include <assert.h>
#include <stdio.h>
#include <os/audio.h>
#include <os/runtime.h>

static int16_t pcm[OS_AUDIO_MAX_FRAMES * 2];
static void wait_ms(unsigned ms) {
    unsigned start = os_milliseconds();
    while ((unsigned)(os_milliseconds() - start) < ms) os_yield();
}
static void play(unsigned frames, int phase) {
    unsigned sent = 0, start = os_milliseconds();
    int full = 0;
    while (sent < frames) {
        unsigned n = frames - sent;
        if (n > OS_AUDIO_MAX_FRAMES) n = OS_AUDIO_MAX_FRAMES;
        for (unsigned i = 0; i < n; ++i) {
            pcm[2*i] = ((sent+i) % 100 < 50 ? 4000 : -4000) * phase;
            pcm[2*i+1] = ((sent+i) % 64 < 32 ? 2000 : -2000) * phase;
        }
        int written = os_audio_write(pcm, n);
        assert(written == 0 || written == (int)n);
        sent += written;
        if (!written) { full = 1; os_yield(); }
        assert((unsigned)(os_milliseconds() - start) < 10000);
    }
    if (frames > 32768) assert(full);
    while (os_audio_queued() > 0) {
        assert((unsigned)(os_milliseconds() - start) < 10000);
        os_yield();
    }
}
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    assert(os_audio_write(pcm, 1) == -1);
    assert(os_audio_open() == 0);
    assert(os_audio_queued() == 0);
    assert(os_audio_write(NULL, 1) == -1);
    assert(os_audio_write((void*)0xa0000, 1) == -1);
    assert(os_audio_write((void*)0xe0000000, 1) == -1);
    assert(os_audio_write((void*)0xefffffff, 1) == -1);
    assert(os_audio_write(pcm, 0) == -1);
    assert(os_audio_write(pcm, OS_AUDIO_MAX_FRAMES + 1) == -1);
    assert(os_audio_write(pcm, ~0u) == -1);
    play(98304, 1); // three full hardware-ring wraps, with backpressure
    wait_ms(200); // underrun must emit silence, not replay the previous sample
    play(3073, -1); // restart after an underrun, including a one-frame write
    wait_ms(200); // let the host backend consume buffered output
    assert(os_audio_close() == 0);
    assert(os_audio_close() == 0);
    assert(os_audio_queued() == -1);
    assert(os_audio_open() == 0 && os_audio_queued() == 0);
    assert(os_audio_close() == 0);
    puts("*** audio: PASS bounds, DMA wrap, backpressure, drain, restart, close");
    return 0;
}
