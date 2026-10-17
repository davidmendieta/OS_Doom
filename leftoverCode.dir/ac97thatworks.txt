#include "ext2.h"
#include "libk.h"
#include "pci.h"
#include "machine.h"
#include "vmm.h"
#include "audio_data.h"
#include "io.h"
#include "ac97.h"
#include "vme.h"

// saved again!!!mak
// Global variables (declared extern in ac97.h)
BufferDescriptor* bdl;
uint32_t nam;   // Native Audio Mixer BAR (BAR0)
uint32_t nabm;  // Native Audio Bus Master BAR (BAR1)
uint32_t sample_index = 0;

// Additional internal variables
static uint32_t* buffers[BDL_ENTRY_COUNT];  // Array of pointers to audio buffers
static bool ac97_initialized = false;

int16_t audio_samples[SAMPLE_RATE * 2];
uint32_t num_samples = SAMPLE_RATE;

// IO Port helper functions
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Initialize AC97 controller and setup for audio playback
uint32_t init_ac97() {
    Debug::printf("Initializing AC97 audio controller\n");
    
    // Find the AC97 controller on the PCI bus
    PciDevice* ac97 = findDeviceVD(0x8086, 0x2415);
    if (!ac97) {
        Debug::printf("AC97 controller not found!\n");
        return 1;
    }
    
    Debug::printf("Found AC97 device at bus=%d, slot=%d, func=%d\n", 
                 ac97->bus, ac97->slot, ac97->function);

    uint8_t bus = ac97->bus;
    uint8_t slot = ac97->slot;
    uint8_t func = ac97->function;

    // Enable I/O space access and bus mastering
    uint32_t regValue = pciConfigRead32(bus, slot, func, 0x04);
    uint16_t command = (uint16_t)(regValue & 0xFFFF);
    command |= 0x05;  // Enable bits 0 (I/O space) and 2 (bus master)
    uint32_t newRegValue = (regValue & 0xFFFF0000) | command;
    pciConfigWrite32(bus, slot, func, 0x04, newRegValue);

    // Verify the command register was properly set
    uint32_t verifyCommand = pciConfigRead32(bus, slot, func, 0x04);
    if ((verifyCommand & 0x05) != 0x05) {
        Debug::printf("Failed to enable I/O space and bus mastering\n");
        return 1;
    }

    // Get the I/O base addresses
    nam = pciConfigRead32(bus, slot, func, 0x10) & ~0xF;
    nabm = pciConfigRead32(bus, slot, func, 0x14) & ~0xF;

    Debug::printf("AC97 mixer base address: 0x%x\n", nam);
    Debug::printf("AC97 bus master base address: 0x%x\n", nabm);

    // Reset the AC97 codec
    outl(nabm + AC97_RESET_REG, 0x02);  // Reset bus master
    outl(nam + AC97_RESET_REG, 0x01);   // Reset codec
    
    // Wait for reset to complete (with timeout)
    int timeout = 1000;
    while (timeout--) {
        uint32_t status = inl(nabm + AC97_GLOBAL_STAT);
        if (status & 0x100) {  // Bit 8 indicates codec is ready
            break;
        }
        
        // Small delay between checks
        for (int i = 0; i < 1000; i++) {
            __asm__ volatile("nop");
        }
    }
    
    if (timeout <= 0) {
        Debug::printf("AC97 reset timeout!\n");
        return 1;
    }
    
    // Finish reset by clearing reset bit
    outl(nam + AC97_RESET_REG, 0x00);
    
    // Setup audio format and sample rate
    outw(nam + 0x2C, 0xBB80); // Set PCM front DAC rate to 48000Hz
    outw(nam + 0x2E, 0x0B0A); // PCM format (16-bit stereo)
    outw(nam + 0x32, 0xBB80); // Set PCM LR DAC rate to 48000Hz
    
    // Set master volume and PCM volume (0x0000 = max, 0x8000 = mute)
    outw(nam + 0x02, 0x0000); // Set master volume to reasonable level
    outw(nam + 0x18, 0x0000); // Set PCM output volume
    
    // Unmute master volume
    uint16_t master_vol = inw(nam + 0x02);
    master_vol &= ~(1 << 15); // Clear mute bit
    outw(nam + 0x02, master_vol);
    
    // Unmute PCM output
    uint16_t pcm_vol = inw(nam + 0x18);
    pcm_vol &= ~(1 << 15); // Clear mute bit
    outw(nam + 0x18, pcm_vol);
    // Allocate memory for Buffer Descriptor List using VMM
    bdl = (BufferDescriptor*)VMM::naive_mmap(
        sizeof(BufferDescriptor) * BDL_ENTRY_COUNT, 
        true, {}, 0, false
    );
    if (!bdl) {
        Debug::printf("Failed to allocate BDL memory\n");
        return 1;
    }
    
    // Initialize BDL to zeros manually (no memset)
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        bdl[i].buffer_addr = 0;
        bdl[i].buffer_len = 0;
        bdl[i].control = 0;
    }
    
    // Get physical address of BDL
    uint32_t bdl_phys = VMM::get_phys_addr((uint32_t)bdl);
    Debug::printf("BDL virtual address: 0x%x, physical address: 0x%x\n", bdl, bdl_phys);
    
    // Allocate buffers using PhysMem
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        // Allocate physical frame for each buffer
        buffers[i] = (uint32_t*)PhysMem::alloc_frame();
        if (!buffers[i]) {
            Debug::printf("Failed to allocate buffer %d\n", i);
            // Cleanup previously allocated buffers
            for (int j = 0; j < i; j++) {
                PhysMem::dealloc_frame((uint32_t)buffers[j]);
            }
            VMM::naive_munmap(bdl, true);
            return 1;
        }
        
        // Initialize buffer with zero (no memset)
        for (uint32_t j = 0; j < BUFFER_SIZE / 4; j++) {
            buffers[i][j] = 0;
        }
        
        // Set up the buffer descriptor
        bdl[i].buffer_addr = (uint32_t)buffers[i];
        bdl[i].buffer_len = BUFFER_SIZE;
        bdl[i].control = (i == (BDL_ENTRY_COUNT - 1)) ? (1 << 15) : 0; // Interrupt on last buffer
        
        Debug::printf("Buffer %d: virt=0x%x, phys=0x%x, len=%d\n", 
                     i, buffers[i], bdl[i].buffer_addr, bdl[i].buffer_len);
    }
    
    // Fill buffers with initial audio data
    fill_ac97_buffers();
    
    // Configure the AC97 DMA engine
    outl(nabm + AC97_PCM_OUT_BDBAR, bdl_phys);
    outb(nabm + AC97_PCM_OUT_LVI, BDL_ENTRY_COUNT - 1); // Last valid index
    
    // Clear any pending status bits
    outb(nabm + AC97_PCM_OUT_STATUS, 0xFF);
    
    // Reset the current index register
    outb(nabm + AC97_PCM_OUT_CIV, 0);
    
    // Set volume levels
    outb(nam + 0x2A, 0x20); // left master vol (0–0x3F)
    outb(nam + 0x2B, 0x20); // right master vol
    
    // Start playback
    outb(nabm + AC97_PCM_OUT_CONTROL, 0x01); // Enable playback
    ac97_initialized = true;
    
    Debug::printf("AC97 initialization complete\n");
    return 0;
}

// Fill all buffers with audio data
void fill_ac97_buffers() {
    if (!ac97_initialized) return;
    
    Debug::printf("Filling AC97 buffers with audio data\n");
    
    sample_index = 0;
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        uint16_t* buf = (uint16_t*)buffers[i];
        
        for (int j = 0; j < BUFFER_SIZE / 2; j += 2) {
            if (sample_index < num_samples * 2) {
                // Ensure we don't exceed audio data bounds
                // buf[j] = (j / 100) % 2 ? 0x7FFF : -0x7FFF; // Left
                // buf[j + 1] = (j / 100) % 2 ? 0x7FFF : -0x7FFF; // Right
                buf[j] = audio_samples[sample_index++];
                buf[j + 1] = audio_samples[sample_index++];
            } else {
                // Loop back to beginning of audio data
                sample_index = 0;
                // buf[j] = (j / 100) % 2 ? 0x7FFF : -0x7FFF; // Left
                // buf[j + 1] = (j / 100) % 2 ? 0x7FFF : -0x7FFF; // Right
                buf[j] = audio_samples[sample_index++];
                buf[j + 1] = audio_samples[sample_index++];
            }
        }
    }
}

// Main playback loop for AC97
void poll_ac97_loop() {
    if (!ac97_initialized) {
        Debug::printf("Cannot poll AC97 - not initialized\n");
        return;
    }
    
    Debug::printf("Starting AC97 polling loop\n");
    
    // Last buffer we filled
    uint8_t last_lvi = BDL_ENTRY_COUNT - 1;
    
    while (true) {
        // Get current buffer being processed by DMA
        uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
        uint8_t status = inb(nabm + AC97_PCM_OUT_STATUS);
        
        // Calculate safe buffer to refill (next buffer)
        uint8_t next_lvi = (last_lvi + 1) % BDL_ENTRY_COUNT;
        
        // Only refill if we won't overwrite the buffer being played
        if (next_lvi != civ) {
            uint16_t* buf = (uint16_t*)buffers[next_lvi];
            
            // Fill buffer with next audio samples
            for (int j = 0; j < BUFFER_SIZE / 2; j += 2) {
                if (sample_index < num_samples * 2) {
                    buf[j] = audio_samples[sample_index++];
                    buf[j + 1] = audio_samples[sample_index++];
                } else {
                    // Loop back to beginning
                    sample_index = 0;
                    buf[j] = audio_samples[sample_index++];
                    buf[j + 1] = audio_samples[sample_index++];
                }
            }
            
            // Set interrupt on completion for the last buffer
            bdl[next_lvi].control = (next_lvi == (BDL_ENTRY_COUNT - 1)) ? (1 << 15) : 0;
            
            // Update LVI register to include this buffer
            outb(nabm + AC97_PCM_OUT_LVI, next_lvi);
            last_lvi = next_lvi;
        }
        
        // Check for FIFO errors
        if (status & 0x10) {
            Debug::printf("AC97 FIFO error detected\n");
            outb(nabm + AC97_PCM_OUT_STATUS, 0x10); // Clear error
            
            // Recovery strategy: restart playback
            outb(nabm + AC97_PCM_OUT_CONTROL, 0x00); // Stop
            
            // Wait for DMA to stop
            for (int i = 0; i < 1000; i++) {
                if (!(inb(nabm + AC97_PCM_OUT_STATUS) & 0x01)) {
                    break;
                }
            }
            
            // Reset CIV and LVI
            outb(nabm + AC97_PCM_OUT_CIV, 0);
            outb(nabm + AC97_PCM_OUT_LVI, BDL_ENTRY_COUNT - 1);
            
            // Restart playback
            outb(nabm + AC97_PCM_OUT_CONTROL, 0x01);
            last_lvi = BDL_ENTRY_COUNT - 1;
        }
        
        // Small delay to prevent CPU hogging
        for (int i = 0; i < 10000; i++);
    }
}

// AC97 interrupt handler
void ac97_interrupt_handler() {
    if (!ac97_initialized) return;
    
    // Read status register
    uint8_t status = inb(nabm + AC97_PCM_OUT_STATUS);
    
    // Handle buffer completion interrupt
    if (status & 0x01) {
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

// Refill buffers function similar to interrupt handler
void refill_buffers() {
    if (!ac97_initialized) return;
    
    uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
    uint8_t next_buffer = (civ + 1) % BDL_ENTRY_COUNT;
    
    uint16_t* buf = (uint16_t*)buffers[next_buffer];
    
    for (int i = 0; i < BUFFER_SIZE / 2; i += 2) {
        if (sample_index < num_samples * 2) {
            buf[i] = audio_samples[sample_index++];     // Left
            buf[i + 1] = audio_samples[sample_index++]; // Right
        } else {
            sample_index = 0;
            buf[i] = audio_samples[sample_index++];     // Left
            buf[i + 1] = audio_samples[sample_index++]; // Right
        }
    }
    
    // Update LVI
    outb(nabm + AC97_PCM_OUT_LVI, next_buffer);
}

// Reset buffers to silence
void reset_buffers() {
    if (!ac97_initialized) return;
    
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        uint16_t* buf = (uint16_t*)buffers[i];
        
        // Zero out the buffer (without memset)
        for (int j = 0; j < BUFFER_SIZE / 2; j++) {
            buf[j] = 0;
        }
    }
    
    sample_index = 0;
}

// Clean up and shutdown AC97
void shutdown_ac97() {
    if (!ac97_initialized) return;
    
    Debug::printf("Shutting down AC97\n");
    
    // Stop playback
    outb(nabm + AC97_PCM_OUT_CONTROL, 0x00);
    
    // Wait for DMA to stop
    for (int i = 0; i < 1000; i++) {
        if (!(inb(nabm + AC97_PCM_OUT_STATUS) & 0x01)) {
            break;
        }
    }
    
    // Free allocated memory
    if (bdl) {
        VMM::naive_munmap(bdl, true);
        bdl = NULL;
    }
    
    // Free buffer memory
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        if (buffers[i]) {
            PhysMem::dealloc_frame((uint32_t)buffers[i]);
            buffers[i] = NULL;
        }
    }
    
    ac97_initialized = false;
    Debug::printf("AC97 shutdown complete\n");
}
