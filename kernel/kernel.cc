#include "debug.h"
#include "ide.h"
#include "ext2.h"
#include "elf.h"
#include "machine.h"
#include "libk.h"
#include "config.h"
#include "shared.h"
#include "ac97.h"

#define SAMPLE_RATE 48000

StrongPtr<Ext2> fs{};

// WAV format header used for loading .wav files if needed
struct WavFmtHeader {
    char     riff_id[4];
    uint32_t riff_sz;
    char     wave_id[4];
    char     fmt_id[4];
    uint32_t fmt_sz;
    uint16_t audio_fmt;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

// Generate a 440Hz test tone into the audio_samples buffer
void generate_test_tone() {
    Debug::printf("*** Generating test sine wave at 440Hz\n");

    num_samples = SAMPLE_RATE; // One second of audio
    sample_index = 0;

    // Precomputed 32-point sine table scaled to 16-bit range
    const int16_t sine_table[32] = {
        0, 6392, 12539, 18204, 23169, 27244, 30272, 32137,
        32767, 32137, 30272, 27244, 23169, 18204, 12539, 6392,
        0, -6392, -12539, -18204, -23169, -27244, -30272, -32137,
        -32767, -32137, -30272, -27244, -23169, -18204, -12539, -6392
    };

    const uint32_t samples_per_cycle = SAMPLE_RATE / 440; // ~109 samples per cycle
    const int16_t amplitude = 16384; // Half max amplitude for 16-bit signed

    for (uint32_t i = 0; i < num_samples; i++) {
        uint32_t position = (i * 32) / samples_per_cycle % 32;
        int16_t sample_value = (sine_table[position] * amplitude) / 32767;

        // Stereo: same sample to both left and right
        audio_samples[i * 2]     = sample_value;
        audio_samples[i * 2 + 1] = sample_value;
    }

    Debug::printf("*** Generated %u samples of sine wave\n", num_samples * 2);
}

// Set up user space arguments on stack
uint32_t* load_args(uint32_t* userEsp, const char** argv) {
    uint32_t argc = 0;
    while (argv[argc] != nullptr) {
        argc++;
    }

    uint32_t length[argc];
    uint32_t sum = 0;
    for (uint32_t i = 0; i < argc; i++) {
        length[i] = K::strlen(argv[i]) + 1;
        sum += length[i];
    }

    uint32_t next = sizeof(char*) * (argc + 2); // argc + argv[] + nullptr
    userEsp -= ((sum + next + 15) / 16) * 16;   // Align stack to 16 bytes

    ((uint32_t*)userEsp)[0] = argc;
    ((uint32_t*)userEsp)[1] = (uint32_t)userEsp + 8;

    for (uint32_t i = 0; i < argc; i++) {
        ((uint32_t*)userEsp)[2 + i] = (uint32_t)userEsp + next;
        memcpy((void*)((char*)userEsp + next), argv[i], length[i]);
        next += length[i];
    }

    return userEsp;
}

/*
Assuming you have a .wav file in your ext2 filesystem, you could do something like:

File* wavFile = fs->root->lookup("test.wav");
if (wavFile) {
    uint32_t sz = wavFile->size();
    uint8_t* buf = (uint8_t*)VMM::naive_mmap(sz, true, {}, 0, false);
    wavFile->read(0, buf, sz);
    load_wav(buf, sz);
}
*/
bool load_wav(const uint8_t* data, size_t size) {
    if (size < 44) {
        Debug::printf("Invalid WAV: file too small\n");
        return false;
    }

    const WavFmtHeader* hdr = (const WavFmtHeader*)data;

    // Compare "RIFF" and "WAVE"
    if (!(hdr->riff_id[0] == 'R' && hdr->riff_id[1] == 'I' &&
          hdr->riff_id[2] == 'F' && hdr->riff_id[3] == 'F')) {
        Debug::printf("Invalid WAV: missing RIFF\n");
        return false;
    }

    if (!(hdr->wave_id[0] == 'W' && hdr->wave_id[1] == 'A' &&
          hdr->wave_id[2] == 'V' && hdr->wave_id[3] == 'E')) {
        Debug::printf("Invalid WAV: missing WAVE\n");
        return false;
    }

    if (hdr->audio_fmt != 1 || hdr->bits_per_sample != 16) {
        Debug::printf("Unsupported WAV format (only 16-bit PCM supported)\n");
        return false;
    }

    bool is_stereo = hdr->channels == 2;
    bool is_mono = hdr->channels == 1;
    if (!is_stereo && !is_mono) {
        Debug::printf("Unsupported channel count: %d\n", hdr->channels);
        return false;
    }

    // Find "data" chunk
    const uint8_t* ptr = data + sizeof(WavFmtHeader);
    const uint8_t* end = data + size;
    const uint8_t* data_chunk = nullptr;
    uint32_t data_size = 0;

    while (ptr + 8 < end) {
        // Check chunk ID manually
        if (ptr[0] == 'd' && ptr[1] == 'a' && ptr[2] == 't' && ptr[3] == 'a') {
            data_size = *(const uint32_t*)(ptr + 4);
            data_chunk = ptr + 8;
            break;
        }

        uint32_t chunk_size = *(const uint32_t*)(ptr + 4);
        ptr += 8 + chunk_size;
    }

    if (!data_chunk) {
        Debug::printf("No data chunk found in WAV\n");
        return false;
    }

    uint32_t total_samples = data_size / 2; // 2 bytes per sample
    uint32_t sample_pairs = is_stereo ? total_samples / 2 : total_samples;
    if (sample_pairs > SAMPLE_RATE) {
        Debug::printf("WAV too long, truncating to 1 second\n");
        sample_pairs = SAMPLE_RATE;
    }

    num_samples = sample_pairs;
    sample_index = 0;

    const int16_t* pcm = (const int16_t*)data_chunk;

    for (uint32_t i = 0; i < sample_pairs; i++) {
        if (is_stereo) {
            audio_samples[i * 2]     = *pcm++; // Left
            audio_samples[i * 2 + 1] = *pcm++; // Right
        } else {
            int16_t mono = *pcm++;
            audio_samples[i * 2]     = mono;   // Left
            audio_samples[i * 2 + 1] = mono;   // Right
        }
    }

    Debug::printf("Loaded WAV: %d samples (%s)\n", num_samples, is_stereo ? "stereo" : "mono");
    return true;
}

// Kernel entry point
void kernelMain(void) {
    generate_test_tone();

    if (init_ac97() == 0) {
        Debug::printf("*** audio init success\n");
        poll_ac97_loop(); // Infinite loop — playback polling
    } else {
        Debug::printf("*** audio init failed\n");
    }

    shutdown_ac97();
    Debug::printf("*** audio shutdown success\n");
}
