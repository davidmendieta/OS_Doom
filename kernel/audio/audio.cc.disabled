#include <stdint.h>
#include <stddef.h>
#include "pci.h"
#include "io.h"
#include "memory.h"
#include "sleep.h"

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
#define BUFFER_SIZE     0x1000  // 4 KB per buffer

struct ac97_bdl_entry {
    uint32_t buffer_addr;
    uint32_t buffer_len;
    uint32_t control;
    uint32_t reserved;
};

static uint32_t bar0, bar1;

static struct ac97_bdl_entry* bdl;
static uint8_t* audio_buffer;

// Simple PCI AC'97 detection
int ac97_init() {
    pci_device_t* dev = pci_find_device(AC97_CLASS, AC97_SUBCLASS, 0);
    if (!dev) {
        return -1; // Not found
    }

    bar0 = pci_read_bar(dev, 0) & ~0x3;
    bar1 = pci_read_bar(dev, 1) & ~0x3;

    // Enable I/O and Bus Mastering
    uint16_t command = pci_read_config_word(dev, PCI_COMMAND);
    command |= (1 << 0) | (1 << 2);
    pci_write_config_word(dev, PCI_COMMAND, command);

    // Reset codec
    outw(0x0002, bar0 + AC97_RESET_REG); // Cold reset
    sleep(10); // Delay to allow reset

    uint16_t status = inw(bar0 + AC97_GLOBAL_STAT);
    if (!(status & (1 << 15))) {
        return -2; // Codec not ready
    }

    return 0;
}

// Setup buffer descriptors
void ac97_setup_buffers() {
    bdl = (struct ac97_bdl_entry*) alloc_dma_mem(sizeof(struct ac97_bdl_entry) * BDL_ENTRY_COUNT);
    audio_buffer = (uint8_t*) alloc_dma_mem(BDL_ENTRY_COUNT * BUFFER_SIZE);

    for (int i = 0; i < BDL_ENTRY_COUNT; ++i) {
        bdl[i].buffer_addr = get_phys_addr(&audio_buffer[i * BUFFER_SIZE]);
        bdl[i].buffer_len = BUFFER_SIZE;
        bdl[i].control = 0x80000000; // Interrupt on completion
        bdl[i].reserved = 0;
    }

    outl(get_phys_addr(bdl), bar1 + AC97_PCM_OUT_BDBAR);
    outb(BDL_ENTRY_COUNT - 1, bar1 + AC97_PCM_OUT_LVI);
}

// Write simple test tone (e.g., silence or sawtooth wave)
void ac97_fill_audio_buffer() {
    for (int i = 0; i < BDL_ENTRY_COUNT * BUFFER_SIZE; i += 2) {
        // Sample: 16-bit signed, stereo interleaved
        audio_buffer[i] = 0x00;
        audio_buffer[i + 1] = 0x00;
    }
}

// Start playback
void ac97_start_playback() {
    outb(0x1, bar1 + AC97_PCM_OUT_CONTROL); // Enable output DMA
}

// Entry point for the driver
void ac97_driver_main() {
    if (ac97_init() != 0) {
        // Error: no device or codec not ready
        return;
    }

    ac97_setup_buffers();
    ac97_fill_audio_buffer();
    ac97_start_playback();
}
