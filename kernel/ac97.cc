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
uint32_t nam;   // Native Audio Mixer BAR (BAR0)
uint32_t nabm;  // Native Audio Bus Master BAR (BAR1)
uint32_t sample_index = 0;

// Array of BufferDescriptors, one for each buffer
BufferDescriptor* bdl;

// Allocate memory for buffers (audio data)
uint32_t* buffers[BDL_ENTRY_COUNT];  // Array of pointers to audio buffers




uint32_t init_ac97() {
    // Initialize the AC'97 controller

    PciDevice* ac97 = findDeviceVD(0x8086, 0x2415);

    Debug::printf("We found our device: 0x%x\n", ac97);

    uint8_t bus = ac97->bus;
    uint8_t slot = ac97->slot;
    uint8_t func = ac97->function;

    // uint16_t command = pciConfigRead16(bus, slot, func, 0x04);
    // pciConfigWrite16(bus, slot, func, 0x04, command | 0x05);

    uint32_t regValue = pciConfigRead32(bus, slot, func, 0x04);
    uint16_t command = (uint16_t)(regValue & 0xFFFF);

    command |= 0x05;  // Modify the command register (e.g., add 0x05)
    uint32_t newRegValue = (regValue & 0xFFFF0000) | (command & 0xFFFF);  // Keep the upper 16 bits the same
    pciConfigWrite32(bus, slot, func, 0x04, newRegValue);

    // get bars
    nam = pciConfigRead32(bus, slot, func, 0x10) & ~0xF;
    nabm = pciConfigRead32(bus, slot, func, 0x14) & ~0xF;

    Debug::printf("We got our nam: 0x%x\n", nam);
    Debug::printf("We found our nabm: 0x%x\n", nabm);

    //reset the codec
    // outl(nabm + AC97_RESET_REG, 0x02);  // Set reset bit
    // Wait for reset to complete
    outl(nam  + AC97_RESET_REG, 0x01);
    for (int i = 0; i < 1000; i++) {
        uint32_t status = inl(nabm + AC97_GLOBAL_STAT);
        if (status & 0x100) {  // Check if ready
            break;
        }
        if (i == 999) {
            Debug::printf("AC97 reset timeout!\n");
            return 1;
        }
    }
    outl(nam  + AC97_RESET_REG, 0x00);

    bdl = (BufferDescriptor*)VMM::naive_mmap(sizeof(BufferDescriptor) * BDL_ENTRY_COUNT, true, {}, 0, false);

    Debug::printf("We mmapped our bdl: 0x%x\n", bdl);

    // 1. Allocate buffers
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        // Allocate memory for each buffer
        buffers[i] = (uint32_t*)PhysMem::alloc_frame();
        buffers[i] = (uint32_t*) VMM::naive_mmap(
            BUFFER_SIZE, true, /*file*/{}, 0, false
        );
        // 2) touch it so the VMM backs it with a real frame
        buffers[i][0] = 0;
        
        Debug::printf("We mmapped our buffer[%d]: 0x%x\n", i, buffers[i]);
        // Set up the corresponding BufferDescriptor
        bdl[i].buffer_addr = get_phys_addr((uint32_t)buffers[i]);
        //bdl[i].buffer_addr = 0xA1000000;
        bdl[i].buffer_len = BUFFER_SIZE;
        bdl[i].control = (i == (BDL_ENTRY_COUNT - 1)) ? (1 << 15) : 0;  // Interrupt on completion for last buffer
        Debug::printf("We set our buffer[%d] addr: 0x%x\n", i, bdl[i].buffer_addr);
        Debug::printf("We have our buffer[%d] length: 0x%x\n", i, bdl[i].buffer_len);
        Debug::printf("We have our buffer[%d] control: 0x%x\n", i, bdl[i].control);
    }

    generate_test_tone();
    fill_ac97_buffers();

    // 2. Set up the BDL (Buffer Descriptor List) in the AC'97 controller
    outb(nam + 0x2A, 0x08); // left master vol (0–0x3F)
    outb(nam + 0x2B, 0x08); // right master vol
    outl(nabm + AC97_PCM_OUT_BDBAR, get_phys_addr((uint32_t)bdl));
    outb(nabm + AC97_PCM_OUT_STATUS, 0xFF); // Clear status
    outb(nabm + AC97_PCM_OUT_CIV, 0);    // Current index (set to 0)
    outb(nabm + AC97_PCM_OUT_LVI, BDL_ENTRY_COUNT - 1); // Last valid index
    outb(nabm + AC97_PCM_OUT_CONTROL, 0x11); // Enable playback
    Debug::printf("finished\n");
    poll_ac97_loop();
    return 0;
}


// plays the audio files in the actual buffers
void poll_ac97_loop() {
    Debug::printf("Starting audio playback loop\n");
    
    // last index we refilled 
    uint8_t last_civ = 0;
    
    while (true) {
        // Read current index value
        uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV); // current index value 
        uint8_t status = inb(nabm + AC97_PCM_OUT_STATUS);   // status flags
        
        // Check if we've moved to a new buffer
        if (civ != last_civ) {
            Debug::printf("Playing buffer %d, status: 0x%x\n", civ, status);
            
            // Calculate which buffer we can now refill (the one we just finished)
            uint8_t buffer_to_refill = last_civ;
            
            // Fill this buffer with new audio data
            uint16_t* buf = (uint16_t*)buffers[buffer_to_refill];
            for (int j = 0; j < (BUFFER_SIZE / 2); j += 2) {
                if (sample_index < num_samples * 2) {
                    buf[j] = audio_samples[sample_index++];
                    buf[j + 1] = audio_samples[sample_index++];
                    
                } else {
                    // Loop back to start of audio if we've reached the end
                    sample_index = 0;
                    buf[j] = audio_samples[sample_index++];
                    buf[j + 1] = audio_samples[sample_index++];
                }
                Debug::printf("Playing buffer %d, value: 0x%x\n", j, buf[j]);

            }
            
            // Update LVI to include our refilled buffer (aliding window logic do we need this? might just have to set in init)
            outb(nabm + AC97_PCM_OUT_LVI, buffer_to_refill);
            
            // Remember the current position
            last_civ = civ;
        }
        
        // Check for errors
        if (status & 0x10) {  // FIFO error
            Debug::printf("AC97 FIFO error detected!\n");
            outb(nabm + AC97_PCM_OUT_STATUS, 0x10);  // Clear error bit
        }
        
        // Small delay to avoid eating 100% CPU
        for (int i = 0; i < 10000; i++);
    }
}

// fills the buffer so it isnt filled with silence
void fill_ac97_buffers() {
    sample_index = 0;
    // Iterate through each buffer descriptor
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        // Get the actual buffer address from the BufferDescriptor
        uint16_t* buf = (uint16_t*)buffers[i];

        // Fill the buffer with audio data (2 bytes per sample, 2 channels)
        for (int j = 0; j < (BUFFER_SIZE / 2); j += 2) {
            if (sample_index < num_samples * 2) {
                buf[j] = audio_samples[sample_index++];     // Left channel
                buf[j + 1] = audio_samples[sample_index++]; // Right channel
                Debug::printf("Filling buffer %d, value: 0x%x\n", j, buf[j]);
            } else {
                buf[j] = buf[j + 1] = 0; // Fill silence after the audio samples have ended
            }
        }
    }
}


// when audio stops we want to interrupt
void ac97_interrupt_handler() {
    uint8_t status = inb(nabm + AC97_PCM_OUT_STATUS);
    
    // Debug
    Debug::printf("AC97 interrupt: status=0x%x\n", status);

    if (status & 0x01) { // Buffer completion interrupt
        // Get current index
        uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
        
        // Calculate which buffer to refill (the one we just finished)
        uint8_t buffer_to_refill = (civ == 0) ? (BDL_ENTRY_COUNT - 1) : (civ - 1);
        
        // Refill this buffer
        uint16_t* buf = (uint16_t*)buffers[buffer_to_refill];
        for (int j = 0; j < (BUFFER_SIZE / 2); j += 2) {
            if (sample_index < num_samples * 2) {
                buf[j] = audio_samples[sample_index++];
                buf[j + 1] = audio_samples[sample_index++];
            } else {
                // Loop back to start of audio
                sample_index = 0;
                buf[j] = audio_samples[sample_index++];
                buf[j + 1] = audio_samples[sample_index++];
            }
        }
        
        // Update LVI to include our refilled buffer
        outb(nabm + AC97_PCM_OUT_LVI, buffer_to_refill);
    }

    // Acknowledge the interrupt by clearing all set bits
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
    uint16_t* buf = (uint16_t*)buffers[next_buffer];

    // Fill the next buffer with silence
    for (int i = 0; i < (BUFFER_SIZE / 2); i++) {
        buf[i] = 0;  // Set all values to 0 to produce silence
    }
}

// when we stop doom 
void shutdown_ac97() {
    outb(nabm + AC97_PCM_OUT_CONTROL, 0x00); // Stop playback
    for (int i = 0; i < 1000; i++) {
        if (!(inb(nabm + AC97_PCM_OUT_STATUS) & 0x01)) {
            break;
        }
    }
    VMM::naive_munmap(bdl, true);
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        PhysMem::dealloc_frame((uint32_t)buffers[i]);
    }
    // Free BDL
}