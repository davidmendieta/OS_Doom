#include <stdint.h>
uint16_t audio_samples[48000 * 2];  // 1 second stereo at 48kHz
uint32_t num_samples = 48000;

void generate_test_tone() {
    const double freq = 440.0;  // A4 note
    const int rate = 48000;
    const double amplitude = 10000;

    for (uint32_t i = 0; i < num_samples; i++) {
        double t = (double)i / rate;
        int16_t sample = (int16_t)(amplitude * 2.0 * 3.14 * freq * t);
        audio_samples[i * 2] = sample;     // Left
        audio_samples[i * 2 + 1] = sample; // Right
    }
}
