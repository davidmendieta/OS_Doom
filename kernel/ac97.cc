#include "ac97.h"
#include "pci.h"
#include "machine.h"
#include "physmem.h"
#include "blocking_lock.h"
#include "threads.h"
#include "../include/os/audio.h"

BufferDescriptor* bdl = nullptr;
uint32_t nam, nabm, sample_index;
int16_t audio_samples[SAMPLE_RATE * 2];
uint32_t num_samples = SAMPLE_RATE;

namespace {
BlockingLock lock;
uint32_t buffers[BDL_ENTRY_COUNT]{};
uint32_t frames_in_slot[BDL_ENTRY_COUNT]{};
unsigned head, tail, count;
bool opened = false, running = false;

void out16(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port) : "memory");
}
uint16_t in16(uint16_t port) {
    uint16_t value;
    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}
void reset_dma() {
    outb(nabm + AC97_PCM_OUT_CONTROL, 0);
    outb(nabm + AC97_PCM_OUT_CONTROL, 2);
    for (unsigned i = 0; i < 10000; ++i)
        if (!(inb(nabm + AC97_PCM_OUT_CONTROL) & 2)) break;
    head = tail = count = 0;
    running = false;
    outl(nabm + AC97_PCM_OUT_BDBAR, uint32_t(bdl));
}
// Read CIV/PICB consistently while DMA can advance independently of the CPU.
int remaining() {
    if (!count) return 0;
    unsigned civ, after;
    uint16_t status, samples;
    do {
        civ = inb(nabm + AC97_PCM_OUT_CIV);
        status = in16(nabm + AC97_PCM_OUT_STATUS);
        samples = in16(nabm + 0x18);
        after = inb(nabm + AC97_PCM_OUT_CIV);
    } while (civ != after);
    while (count && head != civ) { head = (head + 1) % BDL_ENTRY_COUNT; --count; }
    if ((status & 1) && samples == 0) {
        count = 0; // last valid buffer completed; BUP=0 makes the device silent
        return 0;
    }
    unsigned total = samples / 2;
    for (unsigned i = 1; i < count; ++i) total += frames_in_slot[(head + i) % BDL_ENTRY_COUNT];
    return total;
}
}

int Ac97::open() {
    LockGuard guard{lock};
    PciDevice* device = findDeviceVD(0x8086, 0x2415);
    if (!device) return -1;
    if (!opened) {
        nam = getBar(device, 0);
        nabm = getBar(device, 1);
        if (!nam || !nabm || nam > 0xffff || nabm > 0xffff) return -1;
        // Only write command bits; the upper PCI status bits are W1C.
        uint32_t command = pciConfigRead32(device->bus, device->slot, device->function, 4) & 0xffff;
        pciConfigWrite32(device->bus, device->slot, device->function, 4, command | 5);
        outl(nabm + 0x2c, 2); // release cold reset, two-channel output
        bool ready = false;
        for (unsigned i = 0; i < 100000; ++i) {
            if (inl(nabm + AC97_GLOBAL_STAT) & 0x100) { ready = true; break; }
        }
        if (!ready) return -1;
        out16(nam, 0); // codec reset (16-bit register)
        out16(nam + 0x2a, 0); // fixed 48 kHz, variable-rate audio disabled
        out16(nam + AC97_MASTER_VOLUME, 0);
        out16(nam + AC97_PCM_OUT_VOLUME, 0);
        // Low physical frames are identity mapped and outlive user mappings.
        bdl = reinterpret_cast<BufferDescriptor*>(PhysMem::alloc_frame());
        for (unsigned i = 0; i < BDL_ENTRY_COUNT; ++i) {
            buffers[i] = PhysMem::alloc_frame();
            bdl[i] = {buffers[i], 0, 0};
        }
        opened = true;
        Debug::printf("*** audio: AC97 48000 Hz stereo PCM ready\n");
    }
    reset_dma();
    return 0;
}
int Ac97::queued() {
    LockGuard guard{lock};
    return opened ? remaining() : -1;
}
int Ac97::write(const int16_t* pcm, uint32_t frames) {
    LockGuard guard{lock};
    if (!opened || frames == 0 || frames > OS_AUDIO_MAX_FRAMES) return -1;
    remaining();
    if (count >= BDL_ENTRY_COUNT - 1) return 0; // never lap the active descriptor
    if (!count) reset_dma();
    memcpy(reinterpret_cast<void*>(buffers[tail]), pcm, frames * 4);
    frames_in_slot[tail] = frames;
    bdl[tail].buffer_len = frames * 2;
    bdl[tail].control = 0;
    asm volatile("" : : : "memory"); // publish PCM/descriptor before advancing LVI
    outb(nabm + AC97_PCM_OUT_LVI, tail);
    tail = (tail + 1) % BDL_ENTRY_COUNT;
    ++count;
    if (!running) {
        outb(nabm + AC97_PCM_OUT_CONTROL, 1);
        running = true;
    }
    return frames;
}
int Ac97::close() {
    LockGuard guard{lock};
    if (!opened) return 0;
    reset_dma(); // halt DMA before returning physical memory
    for (unsigned i = 0; i < BDL_ENTRY_COUNT; ++i) {
        PhysMem::dealloc_frame(buffers[i]);
        buffers[i] = 0;
    }
    PhysMem::dealloc_frame(uint32_t(bdl));
    bdl = nullptr;
    opened = false;
    return 0;
}

// Compatibility entry points for the original kernel audio demo.
uint32_t init_ac97() { return Ac97::open() == 0 ? 0 : 1; }
void shutdown_ac97() { Ac97::close(); }
void reset_buffers() { if (opened) Ac97::open(); sample_index = 0; }
void refill_buffers() {
    if (!num_samples || num_samples > SAMPLE_RATE || Ac97::queued() >= 4096) return;
    int16_t pcm[OS_AUDIO_MAX_FRAMES * 2];
    for (unsigned i = 0; i < OS_AUDIO_MAX_FRAMES * 2; ++i) {
        if (sample_index >= num_samples * 2) sample_index = 0;
        pcm[i] = audio_samples[sample_index++];
    }
    Ac97::write(pcm, OS_AUDIO_MAX_FRAMES);
}
void fill_ac97_buffers() { sample_index = 0; for (unsigned i = 0; i < 4; ++i) refill_buffers(); }
void poll_ac97_loop() { while (opened) { refill_buffers(); yield(); } }
void ac97_interrupt_handler() {} // PCM producer polls; device interrupts disabled
