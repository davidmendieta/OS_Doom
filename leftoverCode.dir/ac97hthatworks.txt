#ifndef AC97_H
#define AC97_H

#include <stdint.h>

// AC'97 PCI Class and Subclass
#define AC97_CLASS       0x04
#define AC97_SUBCLASS    0x01

// AC'97 Register Offsets
#define AC97_RESET_REG       0x2C
#define AC97_GLOBAL_STAT     0x30
#define AC97_PCM_OUT_BDBAR   0x10
#define AC97_PCM_OUT_CIV     0x14
#define AC97_PCM_OUT_LVI     0x15
#define AC97_PCM_OUT_CONTROL 0x1B
#define AC97_PCM_OUT_STATUS  0x16

// Additional register offsets for volume control
#define AC97_MASTER_VOLUME   0x02
#define AC97_PCM_OUT_VOLUME  0x18
#define AC97_PCM_FRONT_DAC   0x2C
#define AC97_PCM_SURR_DAC    0x2E
#define AC97_PCM_LR_DAC      0x32

// Buffer Descriptor List (BDL) Constants
#define BDL_ENTRY_COUNT 32
#define BUFFER_SIZE     0x1000
#define SAMPLE_RATE 48000


// Buffer Descriptor Structure
struct BufferDescriptor {
    uint32_t buffer_addr;
    uint32_t buffer_len;
    uint32_t control;
} __attribute__((packed));

// Global Variables
extern int16_t audio_samples[SAMPLE_RATE * 2];
extern uint32_t num_samples;
extern BufferDescriptor* bdl;
extern uint32_t nam;  // Native Audio Mixer BAR (BAR0)
extern uint32_t nabm; // Native Audio Bus Master BAR (BAR1)
extern uint32_t sample_index;

// Function Declarations
uint32_t init_ac97();
void poll_ac97_loop();
void fill_ac97_buffers();
void ac97_interrupt_handler();
void refill_buffers();
void reset_buffers();
void shutdown_ac97();

#endif // AC97_H


