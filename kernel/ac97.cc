#include "ext2.h"
#include "libk.h"
#include "pci.h"
#include "machine.h"
#include "vmm.h"
#include "io.h"
#include "ac97.h"
#include "vme.h"
// #include "audio_data.h"

BufferDescriptor* bdl;
uint32_t nam;   // Native Audio Mixer BAR (BAR0)
uint32_t nabm;  // Native Audio Bus Master BAR (BAR1)
uint32_t sample_index = 0;

// Internal state
static uint32_t* buffers[BDL_ENTRY_COUNT];  // pointers to audio buffers
static bool ac97_initialized = false;

int16_t audio_samples[SAMPLE_RATE * 2];
uint32_t num_samples = SAMPLE_RATE;

// IO port access declared inline
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Initialize AC97 controller
uint32_t init_ac97() {

    Debug::printf("Initializing AC97!!!\n");

    // added new num_samples bound check
    if (num_samples > SAMPLE_RATE) {
        Debug::printf("Warning: num_samples exceeds allocated sample space, capping.\n");
        num_samples = SAMPLE_RATE;
    }

    PciDevice* ac97 = findDeviceVD(0x8086, 0x2415);
    if (!ac97) {
        Debug::printf("AC97 controller not found!\n");
        return 1;
    }

    uint8_t bus = ac97->bus;
    uint8_t slot = ac97->slot;
    uint8_t func = ac97->function;

    uint32_t regValue = pciConfigRead32(bus, slot, func, 0x04);
    uint16_t command = (uint16_t)(regValue & 0xFFFF);
    command |= 0x05;
    pciConfigWrite32(bus, slot, func, 0x04, (regValue & 0xFFFF0000) | command);

    if ((pciConfigRead32(bus, slot, func, 0x04) & 0x05) != 0x05) {
        Debug::printf("Failed to enable I/O space and bus mastering\n");
        return 1;
    }

    nam = pciConfigRead32(bus, slot, func, 0x10) & ~0xF;
    nabm = pciConfigRead32(bus, slot, func, 0x14) & ~0xF;

    outl(nabm + AC97_RESET_REG, 0x02);  // Reset bus master
    outl(nam + AC97_RESET_REG, 0x01);   // Reset codec

    int timeout = 1000;
    while (timeout--) {
        if (inl(nabm + AC97_GLOBAL_STAT) & 0x100) break;
        for (int i = 0; i < 1000; i++) __asm__ volatile("nop");
    }

    if (timeout <= 0) {
        Debug::printf("AC97 reset timeout!\n");
        return 1;
    }


    outl(nam + AC97_RESET_REG, 0x00);

    outw(nam + 0x2C, 0xBB80); // 48000Hz
    outw(nam + 0x2E, 0x0B0A); // 16-bit stereo
    outw(nam + 0x32, 0xBB80); // 48000Hz

    outw(nam + 0x02, 0x0000); // Master volume
    outw(nam + 0x18, 0x0000); // PCM output volume

    uint16_t master_vol = inw(nam + 0x02);
    outw(nam + 0x02, master_vol & ~(1 << 15));  // Unmute master

    uint16_t pcm_vol = inw(nam + 0x18);
    outw(nam + 0x18, pcm_vol & ~(1 << 15));     // Unmute PCM

    bdl = (BufferDescriptor*)VMM::naive_mmap(sizeof(BufferDescriptor) * BDL_ENTRY_COUNT, true, {}, 0, false);
    if (!bdl) {
        Debug::printf("Failed to allocate BDL\n");
        return 1;
    }

    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        bdl[i].buffer_addr = 0;
        bdl[i].buffer_len = 0;
        bdl[i].control = 0;
    }

    uint32_t bdl_phys = VMM::get_phys_addr((uint32_t)bdl);
    // Debug::printf("BDL address: virt=0x%x, phys=0x%x\n", bdl, bdl_phys);

    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {

        buffers[i] = (uint32_t*)PhysMem::alloc_frame();
        
        if (!buffers[i]) {
            Debug::printf("Failed to allocate buffer %d\n", i);
            for (int j = 0; j < i; j++) {
                PhysMem::dealloc_frame((uint32_t)buffers[j]);
            }
            VMM::naive_munmap(bdl, true);
            return 1;
        }

        for (uint32_t j = 0; j < BUFFER_SIZE / 4; j++) {
            buffers[i][j] = 0;
        }
        bdl[i].buffer_addr = (uint32_t)buffers[i];
        bdl[i].buffer_len = BUFFER_SIZE;
        bdl[i].control = (i == (BDL_ENTRY_COUNT - 1)) ? (1 << 15) : 0;

        // Debug::printf("Buffer %d: virt=0x%x, phys=0x%x\n", i, buffers[i], bdl[i].buffer_addr);
    }

    fill_ac97_buffers();

    outl(nabm + AC97_PCM_OUT_BDBAR, bdl_phys);
    outb(nabm + AC97_PCM_OUT_LVI, BDL_ENTRY_COUNT - 1);
    outb(nabm + AC97_PCM_OUT_STATUS, 0xFF);
    outb(nabm + AC97_PCM_OUT_CIV, 0);

    outb(nam + 0x2A, 0x20); // Set master left volume
    outb(nam + 0x2B, 0x20); // Set master right volume

    outb(nabm + AC97_PCM_OUT_CONTROL, 0x01); // Start playback
    ac97_initialized = true;

    Debug::printf("AC97 initialization complete\n");
    return 0;
}

void fill_ac97_buffers() {
    if (!ac97_initialized) return;

    // Debug::printf("Filling AC97 buffers\n");

    sample_index = 0;
    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        uint16_t* buf = (uint16_t*)buffers[i];
        for (int j = 0; j < BUFFER_SIZE / 2; j += 2) {
            if (sample_index >= num_samples * 2) sample_index = 0;
            buf[j] = audio_samples[sample_index++];
            buf[j + 1] = audio_samples[sample_index++];
        }
    }
}

void print_amplitude_bar(int16_t amplitude)
{
    const int total_width = 80; // total width of the terminal line
    const int mid = total_width / 2;

    int offset = (amplitude * mid) / 32768; // normalize to terminal width
    int cursor = mid + offset;

    if (cursor < 0)
        cursor = 0;
    if (cursor >= total_width)
        cursor = total_width - 1;

    for (int i = 0; i < total_width; i++) {
        if (i == mid) {
            Debug::printf("|"); // center marker
        } else if (i == cursor) {
            Debug::printf("*"); // wave value
        } else {
            Debug::printf(" ");
        }
    }
    Debug::printf("\n");
}

void poll_ac97_loop() {
    if (!ac97_initialized) return;

    // Debug::printf("Polling AC97 loop\n");

    uint8_t last_lvi = BDL_ENTRY_COUNT - 1;

    while (true) {
        uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
        uint8_t status = inb(nabm + AC97_PCM_OUT_STATUS);
        uint8_t next_lvi = (last_lvi + 1) % BDL_ENTRY_COUNT;

        if (next_lvi != civ) {
            uint16_t* buf = (uint16_t*)buffers[next_lvi];
            for (int j = 0; j < BUFFER_SIZE / 2; j += 2) {
                if (sample_index >= num_samples * 2) sample_index = 0;
                buf[j] = audio_samples[sample_index++];
                buf[j + 1] = audio_samples[sample_index++];
                // print amplitude for the wave -> Wave Visualizer
                print_amplitude_bar(buf[j]);               
            }

            bdl[next_lvi].control = (next_lvi == (BDL_ENTRY_COUNT - 1)) ? (1 << 15) : 0;
            outb(nabm + AC97_PCM_OUT_LVI, next_lvi);
            last_lvi = next_lvi;
        }

        if (status & 0x10) {
            Debug::printf("AC97 FIFO error detected\n");
            outb(nabm + AC97_PCM_OUT_STATUS, 0x10);
            outb(nabm + AC97_PCM_OUT_CONTROL, 0x00);

            for (int i = 0; i < 1000; i++) {
                if (!(inb(nabm + AC97_PCM_OUT_STATUS) & 0x01)) break;
            }

            outb(nabm + AC97_PCM_OUT_CIV, 0);
            outb(nabm + AC97_PCM_OUT_LVI, BDL_ENTRY_COUNT - 1);
            outb(nabm + AC97_PCM_OUT_CONTROL, 0x01);
            last_lvi = BDL_ENTRY_COUNT - 1;
        }

        for (int i = 0; i < 10000; i++);
    }
}

void ac97_interrupt_handler() {
    if (!ac97_initialized) return;

    uint8_t status = inb(nabm + AC97_PCM_OUT_STATUS);

    if (status & 0x01) {
        uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
        uint8_t buffer_to_refill = (civ == 0) ? (BDL_ENTRY_COUNT - 1) : (civ - 1);

        uint16_t* buf = (uint16_t*)buffers[buffer_to_refill];
        for (int j = 0; j < BUFFER_SIZE / 2; j += 2) {
            if (sample_index >= num_samples * 2) sample_index = 0;
            buf[j] = audio_samples[sample_index++];
            buf[j + 1] = audio_samples[sample_index++];
        }

        outb(nabm + AC97_PCM_OUT_LVI, buffer_to_refill);
    }

    outb(nabm + AC97_PCM_OUT_STATUS, status);
}

void refill_buffers() {
    if (!ac97_initialized) return;

    uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
    uint8_t next_buffer = (civ + 1) % BDL_ENTRY_COUNT;

    uint16_t* buf = (uint16_t*)buffers[next_buffer];
    for (int i = 0; i < BUFFER_SIZE / 2; i += 2) {
        if (sample_index >= num_samples * 2) sample_index = 0;
        buf[i] = audio_samples[sample_index++];
        buf[i + 1] = audio_samples[sample_index++];
    }

    outb(nabm + AC97_PCM_OUT_LVI, next_buffer);
}

void reset_buffers() {
    if (!ac97_initialized) return;

    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        uint16_t* buf = (uint16_t*)buffers[i];
        for (int j = 0; j < BUFFER_SIZE / 2; j++) buf[j] = 0;
    }

    sample_index = 0;
}

void shutdown_ac97() {
    if (!ac97_initialized) return;

    Debug::printf("Shutting down AC97\n");

    outb(nabm + AC97_PCM_OUT_CONTROL, 0x00);
    for (int i = 0; i < 1000; i++) {
        if (!(inb(nabm + AC97_PCM_OUT_STATUS) & 0x01)) break;
    }

    if (bdl) {
        VMM::naive_munmap(bdl, true);
        bdl = NULL;
    }

    for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
        if (buffers[i]) {
            PhysMem::dealloc_frame((uint32_t)buffers[i]);
            buffers[i] = NULL;
        }
    }

    ac97_initialized = false;
    Debug::printf("AC97 shutdown complete\n");
}
