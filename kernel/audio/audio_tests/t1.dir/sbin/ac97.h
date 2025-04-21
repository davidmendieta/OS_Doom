#ifndef AC97_H
#define AC97_H

#include "stdint.h"

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

// Buffer Descriptor List (BDL) Constants
#define BDL_ENTRY_COUNT 32
#define BUFFER_SIZE     0x1000

// Buffer Descriptor Structure
struct BufferDescriptor {
    uint32_t buffer_addr;
    uint32_t buffer_len;
    uint32_t control;
} __attribute__((packed));

// Buffer Structure
struct buffer {
    uint32_t* start_addr;
    uint32_t phys_addr;
    uint32_t size;
};

// Global Variables
// extern buffer* buffers[BDL_ENTRY_COUNT];
// extern BufferDescriptor* bdl;
extern uint32_t nam;  // Native Audio Mixer BAR (BAR0)
extern uint32_t nabm; // Native Audio Bus Master BAR (BAR1)

// Function Declarations
uint32_t get_phys_addr(uint32_t vaddr);
uint32_t init_ac97();
void poll_ac97_loop();
void fill_ac97_buffers();
void ac97_interrupt_handler();
//void refill_buffers();
void shutdown_ac97();

#endif // AC97_H