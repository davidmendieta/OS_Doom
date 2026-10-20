/* Host-side parser/lifecycle checks and deterministic PCM reference renderer. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mus_player.h"
static int16_t a[48000 * 2], b[48000 * 2], c[48000 * 2];
static uint8_t *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb"); assert(f);
    assert(!fseek(f, 0, SEEK_END)); long n = ftell(f); assert(n >= 0);
    rewind(f); uint8_t *data = malloc(n ? n : 1); assert(data);
    assert(fread(data, 1, n, f) == (size_t)n); fclose(f); *size = n; return data;
}
static int audible(const int16_t *pcm, unsigned frames) {
    for (unsigned i = 0; i < frames * 2; ++i) if (pcm[i]) return 1;
    return 0;
}
static size_t score(uint8_t *out, const uint8_t *events, unsigned length) {
    memset(out, 0, 16); memcpy(out, "MUS\x1a", 4);
    out[4] = length; out[5] = length >> 8; out[6] = 16; out[8] = 1;
    memcpy(out + 16, events, length); return 16 + length;
}
static void checks(void) {
    const uint8_t notes[] = {0x40,0,0, 0x90,60|128,100,0x81,0x0c, 0x80,60,0x81,0x0c, 0x60};
    uint8_t data[256]; size_t size = score(data, notes, sizeof(notes));
    for (size_t n = 0; n < size; ++n) assert(!mus_register(data, n));
    assert(mus_register(data, size)); mus_volume(15); mus_play(0);
    mus_render(a, 4800); assert(audible(a, 4800)); assert(mus_position() == 4800);
    mus_pause(1); mus_render(b, 480); assert(!audible(b, 480)); assert(mus_position() == 4800);
    mus_pause(0); mus_render(b, 480);
    mus_play(0); mus_render(a, 4800); mus_render(c, 480); assert(!memcmp(b, c, 480 * 4));
    mus_play(0); mus_volume(0); mus_render(a, 4800); assert(!audible(a, 4800)); assert(mus_position() == 4800);
    mus_volume(15); mus_render(a, 4800); assert(audible(a, 4800));
    mus_stop(); mus_render(a, 4800); assert(!audible(a, 4800)); assert(!mus_active());
    mus_play(1); mus_render(a, 4800);
    // Two seconds (280 MUS ticks) per loop, independent of render block size.
    mus_render(c, 48000); mus_render(c, 43200); assert(mus_position() == 96000);
    mus_render(b, 4800); assert(mus_loops() == 1); assert(!memcmp(a, b, 4800 * 4));
    mus_play(0); mus_render(c, 48000); mus_render(c, 48000); mus_render(c, 1); assert(!mus_active());
    // Pan is observable in the rendered channels, not just register writes.
    const uint8_t pan[] = {0x40,4,0, 0x90,60|128,100,14, 0x60};
    assert(mus_register(data, score(data, pan, sizeof(pan)))); mus_play(0); mus_render(a, 4800);
    assert(audible(a, 4800));
    for (unsigned i = 0; i < 4800; ++i) assert(a[i * 2 + 1] == 0);
    // Sustain must keep a released key sounding until the pedal is lifted.
    const uint8_t sustain[] = {0x40,8,127, 0x90,60|128,100,14, 0x80,60,14, 0xc0,8,0,14, 0x60};
    const uint8_t held[] = {0x90,60|128,100,28, 0x80,60,14, 0x60};
    assert(mus_register(data, score(data, sustain, sizeof(sustain)))); mus_play(0); mus_render(a, 14400);
    assert(mus_register(data, score(data, held, sizeof(held)))); mus_play(0); mus_render(b, 14400);
    assert(!memcmp(a, b, 14400 * 4));
    const uint8_t percussion[] = {0x9f,35|128,100,14, 0x60};
    assert(mus_register(data, score(data, percussion, sizeof(percussion)))); mus_play(0); mus_render(a, 4800);
    assert(audible(a, 4800));
    const uint8_t bend[] = {0x90,60|128,100,14, 0xa0,255,14, 0x60};
    const uint8_t straight[] = {0x90,60|128,100,28, 0x60};
    assert(mus_register(data, score(data, bend, sizeof(bend)))); mus_play(0); mus_render(a, 9600);
    assert(mus_register(data, score(data, straight, sizeof(straight)))); mus_play(0); mus_render(b, 9600);
    assert(!memcmp(a, b, 4800 * 4)); assert(memcmp(a + 9600, b + 9600, 4800 * 4));
    const uint8_t retrigger[] = {0x90,60|128,100,7, 0x00,60, 0x90,60|128,100,7, 0x60};
    const uint8_t continuous[] = {0x90,60|128,100,14, 0x60};
    assert(mus_register(data, score(data, retrigger, sizeof(retrigger)))); mus_play(0); mus_render(a, 4800);
    assert(mus_register(data, score(data, continuous, sizeof(continuous)))); mus_play(0); mus_render(b, 4800);
    assert(!memcmp(a, b, 2400 * 4)); assert(memcmp(a + 4800, b + 4800, 2400 * 4));
    const uint8_t bad_type[] = {0x50}, zero_loop[] = {0x60}, bad_delay[] = {0x90,60,0xff,0xff,0xff,0xff,0x7f,0x60};
    assert(!mus_register(data, score(data, bad_type, sizeof(bad_type))));
    assert(!mus_register(data, score(data, zero_loop, sizeof(zero_loop))));
    assert(!mus_register(data, score(data, bad_delay, sizeof(bad_delay))));
    // Deterministic malformed input corpus under AddressSanitizer.
    unsigned random = 0x12345678;
    for (unsigned trial = 0; trial < 4000; ++trial) {
        uint8_t bytes[128];
        for (unsigned i = 0; i < sizeof(bytes); ++i) { random = random * 1664525u + 1013904223u; bytes[i] = random >> 24; }
        if (mus_register(data, score(data, bytes, sizeof(bytes)))) { mus_play(1); mus_render(a, 128); }
    }
    mus_unregister(); mus_unregister();
    puts("Music core passed: bounded parser, timing, loops, pause/resume, mute, stop, pan, sustain, percussion, bend, malformed input.");
}
int main(int argc, char **argv) {
    assert(argc == 2 || argc == 5);
    size_t size; uint8_t *data = read_file(argv[1], &size);
    assert(!mus_init(data, 8)); assert(mus_init(data, size)); free(data);
    if (argc == 2) { checks(); return 0; }
    data = read_file(argv[2], &size); assert(mus_register(data, size)); free(data);
    unsigned frames = strtoul(argv[4], NULL, 10);
    mus_volume(8); mus_play(1);
    FILE *out = fopen(argv[3], "wb"); assert(out);
    while (frames) {
        unsigned n = frames < 48000 ? frames : 48000;
        mus_render(a, n); assert(fwrite(a, 4, n, out) == n); frames -= n;
    }
    fclose(out); mus_unregister(); return 0;
}
