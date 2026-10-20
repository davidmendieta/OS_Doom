#ifndef AC97_H
#define AC97_H

#include <stdint.h>

// AC'97 PCI Class and Subclass identifiers
#define AC97_CLASS         0x04
#define AC97_SUBCLASS      0x01

// AC'97 Mixer and Bus Master Register Offsets
#define AC97_RESET_REG         0x00
#define AC97_GLOBAL_STAT       0x30
#define AC97_PCM_OUT_BDBAR     0x10
#define AC97_PCM_OUT_CIV       0x14
#define AC97_PCM_OUT_LVI       0x15
#define AC97_PCM_OUT_CONTROL   0x1B
#define AC97_PCM_OUT_STATUS    0x16

// Volume control register offsets
#define AC97_MASTER_VOLUME     0x02
#define AC97_PCM_OUT_VOLUME    0x18
#define AC97_PCM_FRONT_DAC     0x2C
#define AC97_PCM_SURR_DAC      0x2E
#define AC97_PCM_LR_DAC        0x32

// DMA Buffer Configuration
#define BDL_ENTRY_COUNT    32
#define BUFFER_SIZE        0x1000
#define SAMPLE_RATE        48000  // 48kHz sample rate

#define WAV_HEADER_SIZE 44

// Buffer Descriptor used by AC'97 DMA engine
struct BufferDescriptor {
    uint32_t buffer_addr;  // Physical address of buffer
    uint16_t buffer_len;   // Number of 16-bit samples (two per stereo frame)
    uint16_t control;      // IOC=bit 15, BUP=bit 14; zero means silence on underrun
} __attribute__((packed));
static_assert(sizeof(BufferDescriptor) == 8, "AC97 hardware descriptor size");

namespace Ac97 {
    int open();
    int write(const int16_t* pcm, uint32_t frames);
    int queued();
    int close();
}

// WAV File Header Struct
struct WAVHeader {
    char riff[4]; // "RIFF"
    uint32_t chunkSize;
    char wave[4]; // "WAVE"
    char fmt[4];  // "fmt "
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4]; // "data"
    uint32_t dataSize;
};

// External globals (defined in ac97.cpp)
extern BufferDescriptor* bdl;
extern uint32_t nam;   // Native Audio Mixer BAR (BAR0)
extern uint32_t nabm;
extern uint32_t sample_index;

extern int16_t audio_samples[SAMPLE_RATE * 2]; // Interleaved stereo audio data
extern uint32_t num_samples;                   // Total number of samples

extern uint32_t init_ac97();
extern void fill_ac97_buffers();
extern void poll_ac97_loop();
extern void ac97_interrupt_handler();
extern void refill_buffers();
extern void reset_buffers();
extern void shutdown_ac97();
extern void get_wav(uint16_t* audio_samples_array, uint32_t* num_samples_out, const char* filename);
// extern void generate_g_major_scale_int(int16_t* buffer, uint32_t* num_samples_out);

#endif
