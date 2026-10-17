// #include <stdint.h>

// #define SAMPLE_RATE 48000
// #define TABLE_SIZE 256
// #define FREQ 440
// #define AMPLITUDE 10000

// uint16_t audio_samples[SAMPLE_RATE * 2];  // stereo
// uint32_t num_samples = SAMPLE_RATE;

// int16_t sine_table[TABLE_SIZE];

// // Fill the table using a fixed-point sine approximation or offline code
// void init_sine_table() {
//     for (int i = 0; i < TABLE_SIZE; i++) {
//         double angle = (2.0 * 3.141592653589793 * i) / TABLE_SIZE;
//         sine_table[i] = (int16_t)(AMPLITUDE * angle);
//     }
//     // You’ll replace this with actual sine values, see below
// }

// void generate_test_tone() {
//     init_sine_table();

//     // Fixed-point phase accumulator
//     uint32_t phase = 0;
//     uint32_t phase_inc = (FREQ * TABLE_SIZE << 16) / SAMPLE_RATE;

//     for (uint32_t i = 0; i < num_samples; i++) {
//         int16_t sample = sine_table[phase >> 16];

//         audio_samples[i * 2] = sample;     // Left
//         audio_samples[i * 2 + 1] = sample; // Right

//         phase += phase_inc;
//         if ((phase >> 16) >= TABLE_SIZE)
//             phase -= (TABLE_SIZE << 16);
//     }
// }
