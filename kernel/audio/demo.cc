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

    #define SAMPLE_RATE 48000
    #define SINE_TABLE_SIZE 64  // Reduced size for simplicity

    // Precomputed sine table (one full cycle, scaled to 16-bit range)
    int16_t sine_table[SINE_TABLE_SIZE] = {
        0,   182,   365,   548,   730,   912,  1093,  1274,
        1454,  1634,  1813,  1991,  2169,  2346,  2522,  2697,
        2872,  3046,  3219,  3391,  3562,  3732,  3902,  4071,
        4239,  4406,  4572,  4737,  4901,  5065,  5227,  5388,
        5549,  5708,  5867,  6024,  6180,  6336,  6490,  6643,
        6796,  6947,  7097,  7246,  7394,  7541,  7686,  7830,
        7974,  8117,  8258,  8398,  8537,  8675,  8811,  8947,
        9082,  9215,  9347,  9478,  9608,  9737,  9864,  9990
    };

    // Frequencies for G major scale in Hz
    const uint16_t g_major_scale[8] = {
        392, 440, 494, 523, 587, 659, 740, 784
    };

    // Generate G major scale samples (stereo interleaved)
    const uint32_t duration_ms = 500; // 0.5 sec per note
    const uint32_t samples_per_note = (SAMPLE_RATE * duration_ms) / 1000;
    const uint32_t max_samples = SAMPLE_RATE * 2;
    uint32_t index = 0;

    for (int note = 0; note < 8 && index < max_samples; note++) {
        uint32_t phase = 0;
        // Increase phase precision for smoother transitions
        uint32_t phase_step = (g_major_scale[note] * SINE_TABLE_SIZE * 65536) / SAMPLE_RATE; // 16-bit phase precision

        for (uint32_t i = 0; i < samples_per_note; i++) {
            // Use linear interpolation between the two nearest points
            uint32_t table_idx = (phase >> 16) % SINE_TABLE_SIZE;  // Index into sine table (16-bit precision)
            uint32_t next_idx = (table_idx + 1) % SINE_TABLE_SIZE;

            int16_t sample1 = sine_table[table_idx];
            int16_t sample2 = sine_table[next_idx];

            // Linear interpolation (fractional part of phase)
            int t = (phase >> 8) & 0xFF; // Fractional part of phase
            int16_t interpolated_sample = sample1 + ((sample2 - sample1) * t) / 256;

            // Store the interpolated sample
            audio_samples[index++] = interpolated_sample; // Left
            audio_samples[index++] = interpolated_sample; // Right

            phase += phase_step;

            if (index >= max_samples) break;
        }
    }

    num_samples = index / 2;

    Debug::printf("*** Generated %u samples of sine wave\n", num_samples * 2);
}



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
void audio_demo(void) {
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
