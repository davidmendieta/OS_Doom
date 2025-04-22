#include "ext2.h"
#include "libk.h"
#include "pci.h"
#include "machine.h"
#include "vmm.h"
#include "audio_data.h"
#include "io.h"
#include "ac97.h"


#define AC97_CLASS       0x04
#define AC97_SUBCLASS    0x01

#define AC97_RESET_REG   0x2C
#define AC97_GLOBAL_STAT 0x30

#define AC97_PCM_OUT_BDBAR   0x10
#define AC97_PCM_OUT_CIV     0x14
#define AC97_PCM_OUT_LVI     0x15
#define AC97_PCM_OUT_CONTROL 0x1B
#define AC97_PCM_OUT_STATUS  0x16

#define BDL_ENTRY_COUNT 32
#define BUFFER_SIZE     0x1000 

// struct that describes each buffer
// struct BufferDescriptor {
//     uint32_t buffer_addr;  // Physical address of buffer
//     uint32_t buffer_len;   // Buffer size in bytes
//     uint32_t control;      // Flags (bit 15 = interrupt on completion)
// } __attribute__((packed)); // Ensure no padding added


// // struct for our buffer
// struct buffer {
//     uint32_t* start_addr;
//     uint32_t phys_addr;       
//     uint32_t size; 
// };


// Buffer Descriptor List (used by AC'97 DMA engine)
// BufferDescriptor* bdl[BDL_ENTRY_COUNT];  // array of 32 descriptors

// Base I/O addresses for AC'97
/* 
BAR0 - Native Audio Mixer Base Address and BAR1 - Native Audio Bus Master Base Address. Both these BARs are I/O. Before any work with card, 
you must set bit 0 and bit 2 in PCI Command Register to enable I/O space and bus mastering.
*/
uint32_t nam = 0;   // Native Audio Mixer BAR (BAR0)
uint32_t nabm = 0;  // Native Audio Bus Master BAR (BAR1)
uint32_t sample_index = 0;

// Array of BufferDescriptors, one for each buffer
BufferDescriptor bdl[BDL_ENTRY_COUNT];

// Allocate memory for buffers (audio data)
uint32_t* buffers[BDL_ENTRY_COUNT];  // Array of pointers to audio buffers

uint32_t init_ac97() {
    // Initialize the AC'97 controller

    // 1. Allocate buffers
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        // Allocate memory for each buffer
        buffers[i] = (uint32_t*)VMM::naive_mmap(BUFFER_SIZE, false, {}, 0, false);

        // Set up the corresponding BufferDescriptor
        // bdl[i].buffer_addr = get_phys_addr((uint32_t)buffers[i]);
        bdl[i].buffer_addr = 0xA1000000;
        bdl[i].buffer_len = BUFFER_SIZE;
        bdl[i].control = (i == (BDL_ENTRY_COUNT - 1)) ? (1 << 15) : 0;  // Interrupt on completion for last buffer
    }

    // 2. Set up the BDL (Buffer Descriptor List) in the AC'97 controller
    outl(nabm + AC97_PCM_OUT_BDBAR, (uint32_t)bdl);
    outb(nabm + AC97_PCM_OUT_CIV, 0);    // Current index (set to 0)
    outb(nabm + AC97_PCM_OUT_LVI, BDL_ENTRY_COUNT - 1); // Last valid index
    outb(nabm + AC97_PCM_OUT_STATUS, 0xFF); // Clear status
    outb(nabm + AC97_PCM_OUT_CONTROL, 0x01); // Enable playback

    return 0;
}


// plays the audio files in the actual buffers
void poll_ac97_loop() {
    uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);   // current index
    uint8_t lvi = inb(nabm + AC97_PCM_OUT_LVI);   // last valid index
	Debug::printf("trying to play audio\n");
    while (true) {
        civ = inb(nabm + AC97_PCM_OUT_CIV);

        if (civ == lvi) {
            // Loop playback: update LVI to next buffer
            uint8_t new_lvi = (lvi + 1) % BDL_ENTRY_COUNT;

            // Optionally refill buffer here (or rotate audio data)

            outb(nabm + AC97_PCM_OUT_LVI, new_lvi);
            lvi = new_lvi;
        }
    }
}

// fills the buffer so it isnt filled with silence
void fill_ac97_buffers() {
    // Iterate through each buffer descriptor
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        // Get the actual buffer address from the BufferDescriptor
        uint16_t* buf = (uint16_t*)bdl[i].buffer_addr;

        // Fill the buffer with audio data (2 bytes per sample, 2 channels)
        for (int j = 0; j < (BUFFER_SIZE / 2); j += 2) {
            if (sample_index < num_samples * 2) {
                buf[j] = audio_samples[sample_index++];     // Left channel
                buf[j + 1] = audio_samples[sample_index++]; // Right channel
            } else {
                buf[j] = buf[j + 1] = 0; // Fill silence after the audio samples have ended
            }
        }
    }
}


// when audio stops we want to interrupt
void ac97_interrupt_handler() {
    uint8_t status = inb(nabm + AC97_PCM_OUT_STATUS);

    if (status & 0x01) { // Buffer completion interrupt
        reset_buffers();
    }

    // Acknowledge the interrupt
    outb(nabm + AC97_PCM_OUT_STATUS, status);
}

// wtf is sample index
// keep track of index while reading or something so we can use it and refill the buffer???
// refill buffer once we finish playing the buffers, so it keeps playing

// void refill_buffers() {
//     uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
//     uint8_t next_buffer = (civ + 1) % BDL_ENTRY_COUNT;

//     uint16_t* buf = (uint16_t*)buffers[next_buffer]->start_addr;

//     for (int i = 0; i < BUFFER_SIZE / 2; i += 2) { // 2 bytes per sample, 2 channels
//         if (sample_index < num_samples * 2) {
//             buf[i] = audio_samples[sample_index++];     // Left
//             buf[i + 1] = audio_samples[sample_index++]; // Right
//         } else {
//             buf[i] = buf[i + 1] = 0; // Fill silence after end
//         }
//     }
// }

// so it plays silence when a buffer isnt being used
void reset_buffers() {
    uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);  // Current index (not used here)
    uint8_t next_buffer = (civ + 1) % BDL_ENTRY_COUNT;  // Next buffer to reset

    // Get the address of the next buffer from the BufferDescriptor
    uint16_t* buf = (uint16_t*)bdl[next_buffer].buffer_addr;

    // Fill the next buffer with silence
    for (int i = 0; i < (BUFFER_SIZE / 2); i++) {
        buf[i] = 0;  // Set all values to 0 to produce silence
    }
}

// when we stop doom 
void shutdown_ac97() {
    outb(nabm + AC97_PCM_OUT_CONTROL, 0x00); // Stop playback

    // Free BDL
}