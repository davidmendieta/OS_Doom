/* MUS sequencer and GENMIDI-to-OPL3 instrument adapter.
 * Register layout/frequency and volume tables follow Chocolate Doom's
 * i_oplmusic.c (Id Software / Simon Howard, GPL-2.0-or-later).
 * This adapter is also distributed under GPL-2.0-or-later.
 */
#include <stdlib.h>
#include <string.h>
#include "mus_player.h"
#include "opl_tables.h"
#include "third_party/nuked-opl3/opl3.h"

#define INSTRUMENTS 175
#define VOICES 18
struct event { uint32_t tick; uint8_t type, channel, a, b; };
struct channel { unsigned instrument, volume, expression, pan, sustain; int bend; };
struct voice {
    int used, held, channel, key, note, instrument, part, velocity;
    unsigned frequency, age;
};
static uint8_t instruments[INSTRUMENTS][36];
static opl3_chip chip;
static struct channel channels[16];
static struct voice voices[VOICES];
static struct event *events;
static unsigned event_count, cursor, loops, age;
static uint64_t position, duration;
static int initialized, playing, paused, looping, volume = 8;
static const unsigned operators[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};

static unsigned le16(const uint8_t *p) { return p[0] | (p[1] << 8); }
// Preserve hardware write spacing: back-to-back key-off/key-on must reach the
// envelope generator on separate samples so repeated notes retrigger.
static void reg(unsigned r, unsigned v) { OPL3_WriteRegBuffered(&chip, r, v); }
static unsigned bank(int voice) { return voice >= 9 ? 0x100 : 0; }
static const uint8_t *patch(struct voice *v) { return instruments[v->instrument] + 4 + v->part * 16; }

static void key_off(unsigned i) {
    reg(bank(i) + 0xb0 + i % 9, voices[i].frequency >> 8);
    voices[i].used = voices[i].held = 0;
}
static void reset(void) {
    OPL3_Reset(&chip, MUS_RATE);
    reg(0x105, 1); // OPL3, eighteen independent two-operator voices
    reg(0x01, 0x20); // waveform selection
    reg(0xbd, 0); // percussion is played with GENMIDI melodic patches
    memset(voices, 0, sizeof(voices));
    for (int i = 0; i < 16; ++i)
        channels[i] = (struct channel){.volume = 100, .expression = 127, .pan = 64};
    age = 0;
}
int mus_init(const void *bank_data, size_t length) {
    mus_unregister();
    initialized = 0;
    if (!bank_data || length < 8 + sizeof(instruments) || memcmp(bank_data, "#OPL_II#", 8)) return 0;
    memcpy(instruments, (const uint8_t *)bank_data + 8, sizeof(instruments));
    reset();
    initialized = 1;
    return 1;
}
void mus_stop(void) {
    playing = paused = 0;
    if (initialized) reset();
}
void mus_unregister(void) {
    mus_stop();
    free(events);
    events = NULL;
    event_count = cursor = 0;
    position = duration = 0;
}
int mus_register(const void *data, size_t length) {
    mus_unregister();
    const uint8_t *bytes = data;
    if (!initialized || !bytes || length < 16 || memcmp(bytes, "MUS\x1a", 4)) return 0;
    unsigned size = le16(bytes + 4), start = le16(bytes + 6), instruments_used = le16(bytes + 12);
    if (!size || start < 16u + 2 * instruments_used || start > length || size > length - start || le16(bytes + 8) > 15)
        return 0;
    // Each event consumes at least one byte. Allocate a bounded worst case.
    events = calloc(size, sizeof(*events));
    if (!events) return 0;
    unsigned end = start + size, p = start, velocity[16];
    for (int i = 0; i < 16; ++i) velocity[i] = 127;
    uint32_t tick = 0;
    while (p < end) {
        unsigned descriptor = bytes[p++], type = (descriptor >> 4) & 7, channel = descriptor & 15;
        struct event event = {.tick = tick, .type = type, .channel = channel};
        if (type == 6) {
            if (!tick) break; // refuse zero-duration loops
            events[event_count++] = event;
            duration = (uint64_t)tick * MUS_RATE / 140;
            return 1;
        }
        if (type > 4 || p == end) break;
        event.a = bytes[p++];
        if (type == 0 && event.a > 127) break;
        if (type == 1) {
            if (event.a & 128) {
                if (p == end || bytes[p] > 127) break;
                velocity[channel] = bytes[p++];
            }
            event.a &= 127;
            event.b = velocity[channel];
        } else if (type == 3) {
            if (event.a < 10 || event.a > 14) break;
        } else if (type == 4) {
            if (event.a > 9 || p == end || bytes[p] > 127) break;
            event.b = bytes[p++];
        }
        events[event_count++] = event;
        if (descriptor & 128) {
            uint32_t delay = 0;
            unsigned groups = 0, byte;
            do {
                if (p == end || ++groups > 4) goto invalid;
                byte = bytes[p++];
                delay = (delay << 7) | (byte & 127);
            } while (byte & 128);
            if (delay > UINT32_MAX - tick) break;
            tick += delay;
        }
    }
invalid:
    mus_unregister();
    return 0;
}
void mus_play(int loop) {
    if (!initialized || !events) return;
    reset();
    position = cursor = loops = 0;
    looping = !!loop;
    playing = 1;
    paused = 0;
}
void mus_pause(int pause) { paused = !!pause; }
void mus_volume(int value) { volume = value < 0 ? 0 : value > 15 ? 15 : value; }
int mus_active(void) { return playing && !paused; }
uint64_t mus_position(void) { return position; }
unsigned mus_loops(void) { return loops; }

static void frequency(unsigned i) {
    struct voice *v = &voices[i];
    const uint8_t *instrument = instruments[v->instrument];
    int note = v->note;
    if (!(le16(instrument) & 1)) note += (int16_t)le16(patch(v) + 14);
    while (note < 0) note += 12;
    while (note > 95) note -= 12;
    int index = 64 + 32 * note + channels[v->channel].bend;
    if (v->part) index += instrument[2] / 2 - 64;
    if (index < 0) index = 0;
    unsigned value;
    if (index < 284) value = frequency_curve[index];
    else {
        unsigned octave = (index - 284) / 384;
        if (octave > 7) octave = 7;
        value = frequency_curve[284 + (index - 284) % 384] | (octave << 10);
    }
    v->frequency = value;
    reg(bank(i) + 0xa0 + i % 9, value & 255);
    reg(bank(i) + 0xb0 + i % 9, (value >> 8) | 0x20);
}
static void levels(unsigned i) {
    struct voice *v = &voices[i];
    struct channel *c = &channels[v->channel];
    const uint8_t *data = patch(v);
    unsigned gain = volume_mapping_table[c->volume * c->expression / 127];
    unsigned level = 63 - ((volume_mapping_table[v->velocity] * (2 * (gain + 1))) >> 9);
    unsigned op = bank(i) + operators[i % 9];
    reg(0x40 + op + 3, (data[11] & 0xc0) | level);
    unsigned mod = data[5] & 63;
    if ((data[6] & 1) && mod < level) mod = level;
    reg(0x40 + op, (data[4] & 0xc0) | mod);
    unsigned pan = c->pan < 32 ? 0x10 : c->pan > 95 ? 0x20 : 0x30;
    reg(bank(i) + 0xc0 + i % 9, (data[6] & 15) | pan);
}
static void operator_data(unsigned op, const uint8_t *data) {
    reg(0x20 + op, data[0]);
    reg(0x60 + op, data[1]);
    reg(0x80 + op, data[2]);
    reg(0xe0 + op, data[3] & 7);
}
static void note_on(unsigned channel, unsigned key, unsigned velocity) {
    if (!velocity) return;
    int instrument = channel == 15 ? 128 + (int)key - 35 : channels[channel].instrument;
    if (instrument < 0 || instrument >= INSTRUMENTS || (channel == 15 && (key < 35 || key > 81))) return;
    unsigned flags = le16(instruments[instrument]);
    int note = flags & 1 ? instruments[instrument][3] : channel == 15 ? 60 : key;
    for (int part = 0; part < ((flags & 4) ? 2 : 1); ++part) {
        unsigned slot = 0;
        // Reuse released slots, otherwise steal the oldest allocated voice.
        for (unsigned i = 0; i < VOICES; ++i) {
            if (!voices[i].used) { slot = i; break; }
            if (voices[i].age < voices[slot].age) slot = i;
        }
        key_off(slot);
        voices[slot] = (struct voice){.used = 1, .held = 1, .channel = channel,
            .key = key, .note = note, .instrument = instrument, .part = part,
            .velocity = velocity, .age = ++age};
        const uint8_t *data = patch(&voices[slot]);
        unsigned op = bank(slot) + operators[slot % 9];
        operator_data(op, data);
        operator_data(op + 3, data + 7);
        levels(slot);
        frequency(slot);
    }
}
static void notes_off(unsigned channel, int key, int immediate) {
    for (unsigned i = 0; i < VOICES; ++i) {
        struct voice *v = &voices[i];
        if (!v->used || v->channel != (int)channel || (key >= 0 && v->key != key)) continue;
        v->held = 0;
        if (immediate || !channels[channel].sustain) key_off(i);
        if (immediate) { // all-sounds-off also silences the release envelope
            unsigned op = bank(i) + operators[i % 9];
            reg(0x40 + op, 63);
            reg(0x40 + op + 3, 63);
        }
    }
}
static void control(unsigned ch, unsigned number, unsigned value) {
    struct channel *c = &channels[ch];
    switch (number) {
    case 0: c->instrument = value; break;
    case 3: c->volume = value; break;
    case 4: c->pan = value; break;
    case 5: c->expression = value; break;
    case 8: c->sustain = value >= 64; break;
    case 10: notes_off(ch, -1, 1); break;
    case 11: case 12: case 13: notes_off(ch, -1, 0); break;
    case 14: c->expression = 127; c->pan = 64; c->bend = 0; c->sustain = 0; break;
    default: break; // bank select, reverb, chorus and soft pedal have no OPL equivalent
    }
    for (unsigned i = 0; i < VOICES; ++i) {
        if (!voices[i].used || voices[i].channel != (int)ch) continue;
        if (!c->sustain && !voices[i].held) key_off(i);
        else { levels(i); if (number == 14) frequency(i); }
    }
}
static void dispatch(const struct event *event) {
    switch (event->type) {
    case 0: notes_off(event->channel, event->a, 0); break;
    case 1:
        if (!event->b) notes_off(event->channel, event->a, 0);
        else note_on(event->channel, event->a, event->b);
        break;
    case 2:
        channels[event->channel].bend = ((int)event->a - 128) / 2;
        for (unsigned i = 0; i < VOICES; ++i)
            if (voices[i].used && voices[i].channel == event->channel) frequency(i);
        break;
    case 3: case 4: control(event->channel, event->a, event->b); break;
    }
}
void mus_render(int16_t *stereo, unsigned frames) {
    memset(stereo, 0, frames * 2 * sizeof(*stereo));
    if (!mus_active()) return;
    for (unsigned frame = 0; frame < frames; ++frame) {
        while (cursor < event_count && (uint64_t)events[cursor].tick * MUS_RATE / 140 <= position) {
            const struct event *event = &events[cursor++];
            if (event->type == 6) {
                if (!looping) { mus_stop(); return; }
                reset();
                cursor = 0;
                position -= duration;
                ++loops;
            } else dispatch(event);
        }
        int16_t sample[2];
        OPL3_GenerateResampled(&chip, sample);
        // Leave headroom for simultaneous weapon/monster sound effects.
        stereo[frame * 2] = sample[0] * volume / 30;
        stereo[frame * 2 + 1] = sample[1] * volume / 30;
        ++position;
    }
}
