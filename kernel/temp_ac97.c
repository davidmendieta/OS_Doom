// #include "ext2.h"
// #include "libk.h"
// #include "pci.h"
// #include "machine.h"
// #include "vmm.h"
// #include "audio_data.h"
// #include "io.h"
// #include "ac97.h"


// #define AC97_CLASS       0x04
// #define AC97_SUBCLASS    0x01

// #define AC97_RESET_REG   0x2C
// #define AC97_GLOBAL_STAT 0x30

// #define AC97_PCM_OUT_BDBAR   0x10
// #define AC97_PCM_OUT_CIV     0x14
// #define AC97_PCM_OUT_LVI     0x15
// #define AC97_PCM_OUT_CONTROL 0x1B
// #define AC97_PCM_OUT_STATUS  0x16

// #define BDL_ENTRY_COUNT 32
// #define BUFFER_SIZE     0x1000 

// // struct that describes each buffer
// // struct BufferDescriptor {
// //     uint32_t buffer_addr;  // Physical address of buffer
// //     uint32_t buffer_len;   // Buffer size in bytes
// //     uint32_t control;      // Flags (bit 15 = interrupt on completion)
// // } __attribute__((packed)); // Ensure no padding added


// // // struct for our buffer
// // struct buffer {
// //     uint32_t* start_addr;
// //     uint32_t phys_addr;       
// //     uint32_t size; 
// // };

// BufferDescriptor* buffers[BDL_ENTRY_COUNT];

// // Buffer Descriptor List (used by AC'97 DMA engine)
// // BufferDescriptor* bdl[BDL_ENTRY_COUNT];  // array of 32 descriptors

// // Base I/O addresses for AC'97
// /* 
// BAR0 - Native Audio Mixer Base Address and BAR1 - Native Audio Bus Master Base Address. Both these BARs are I/O. Before any work with card, 
// you must set bit 0 and bit 2 in PCI Command Register to enable I/O space and bus mastering.
// */
// uint32_t nam = 0;   // Native Audio Mixer BAR (BAR0)
// uint32_t nabm = 0;  // Native Audio Bus Master BAR (BAR1)
// uint32_t sample_index = 0;

// uint32_t get_phys_addr(uint32_t vaddr) {
//     uint32_t pd_index = (vaddr >> 22) & 0x3FF; // Extract page directory index
//     uint32_t pt_index = (vaddr >> 12) & 0x3FF; // Extract page table index

//     // Get the current page directory
//     uint32_t* pd = (uint32_t*)VMM::get_pd();
//     if (!pd) {
//         return 0; // Page directory not available
//     }

//     // Check if the page directory entry is present
//     uint32_t pd_entry = pd[pd_index];
//     if (!(pd_entry & 0x1)) { // Check the present bit
//         return 0; // Page directory entry not present
//     }

//     // Get the page table base address from the page directory entry
//     uint32_t* pt = (uint32_t*)((pd_entry & ~0xFFF)); // Mask out flags
//     if (!pt) {
//         return 0; // Page table not available
//     }

//     // Check if the page table entry is present
//     uint32_t pt_entry = pt[pt_index];
//     if (!(pt_entry & 0x1)) { // Check the present bit
//         return 0; // Page table entry not present
//     }

//     // Get the page frame address from the page table entry
//     uint32_t page_frame = pt_entry & ~0xFFF; // Mask out flags

//     // Calculate the physical address
//     return page_frame | (vaddr & 0xFFF); // Add the page offset
// }

// uint32_t init_ac97() {
//     // 1. Find AC97 PCI device (assume bus, slot, func already known)
// 	Debug::printf("im here in inti\n");
//     PciDevice* ac97 = findDeviceVD(0x8086, 0x100e);
// 	Debug::printf("found device\n");

//     uint8_t bus = ac97->bus;
//     uint8_t slot = ac97->slot;
//     uint8_t func = ac97->function;
// 	Debug::printf("im here in inti 1\n");

//     // 2. Read BARs
//     // nam = getBar();
//     nam = pciConfigRead32(bus, slot, func, 0x10) & ~0xF;
//     nabm = pciConfigRead32(bus, slot, func, 0x14) & ~0xF;
// 	Debug::printf("im here in inti 2\n");

//     // 3. Enable I/O and Bus Mastering
//     uint16_t cmd = pciConfigRead32(bus, slot, func, 0x04);
//     cmd |= 0x5;
//     pciConfigWrite32(bus, slot, func, 0x04, cmd);
// 	Debug::printf("im here in inti 3\n");

//     // 4. Reset AC97
//     outl(nam + AC97_RESET_REG, 0x1);
//     for (int i = 0; i < 100000; i++) {}  // crude delay
//     outl(nam + AC97_RESET_REG, 0x0);
// 	Debug::printf("im here in inti 4\n");

//     // 5. Wait for codec ready
//     while (!(inl(nam + AC97_GLOBAL_STAT) & (1 << 15))) {
//         // Debug::printf("*** waiting for codec\n");
//     }
// 	Debug::printf("im here in inti 5\n");


//     // 6. Allocate buffers
//     for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
		
// 		buffers[i] = (BufferDescriptor*)VMM::naive_mmap(sizeof(BufferDescriptor), false, {}, 0, false);

//         buffers[i]->size = BUFFER_SIZE;
// 		buffers[i]->start_addr = (uint32_t*)VMM::naive_mmap(BUFFER_SIZE, false, {}, 0, false);
// 		Debug::printf("allocating buffer: 0x%x\n", buffers[i]->start_addr);
//         // Fill with silence
//         for (int j = 0; j < BUFFER_SIZE / 4; j++)
//             buffers[i]->start_addr[j] = 0;
//     }
// 	Debug::printf("im here in inti 6\n");


//     // testing audio that isnt the doom stream
//     // extern void generate_test_tone();
//     // extern uint16_t audio_samples[];
//     // extern uint32_t num_samples;
//     generate_test_tone();
//     fill_ac97_buffers();
// 	Debug::printf("umm idk like addr: 0x%x\n", buffers[0]->start_addr);
// 	uint32_t bdl_phys = get_phys_addr((uint32_t)buffers[0]->start_addr);

//     // // 7. Allocate and set up BDL (buffer descriptor list)
//     // uint32_t bdl_phys = get_phys_addr((uint32_t)bdl);
    
// 	// Debug::printf("allocating bdl\n");
//     // for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
//     //     bdl[i].buffer_addr = buffers[i]->phys_addr;
//     //     bdl[i].buffer_len = BUFFER_SIZE;
//     //     bdl[i].control = (i == (BDL_ENTRY_COUNT - 1)) ? (1 << 15) : 0;
// 	// 	Debug::printf("bdl[%d] = %x\n", i, bdl[i].buffer_addr);
//     // }
// 	Debug::printf("im here in inti 7\n");

//     // 8. Program AC'97 controller
//     outb(nam + 0x20, 0x00);
//     outb(nam + 0x2A, 0x80);  // Master volume control for left and right channel (0x80 is max)
//     outb(nam + 0x2B, 0x80);
//     outb(nabm + AC97_PCM_OUT_CONTROL, 0x05);
// 	Debug::printf("physical addr we got: 0x%x\n", bdl_phys);
//     outl(nabm + AC97_PCM_OUT_BDBAR, 0xA1000000); // Set BDL base address
//     outb(nabm + AC97_PCM_OUT_CIV, 0);
//     outb(nabm + AC97_PCM_OUT_LVI, BDL_ENTRY_COUNT - 1);
//     outb(nabm + AC97_PCM_OUT_STATUS, 0xFF); // clear
//     outb(nabm + AC97_PCM_OUT_CONTROL, 0x01); // run + interrupt enable
// 	Debug::printf("im here in inti 8\n");

//     return 0;
// }

// // plays the audio files in the actual buffers
// void poll_ac97_loop() {
//     uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);   // current index
//     uint8_t lvi = inb(nabm + AC97_PCM_OUT_LVI);   // last valid index
// 	Debug::printf("trying to play audio\n");
//     while (true) {
//         civ = inb(nabm + AC97_PCM_OUT_CIV);

//         if (civ == lvi) {
//             // Loop playback: update LVI to next buffer
//             uint8_t new_lvi = (lvi + 1) % BDL_ENTRY_COUNT;

//             // Optionally refill buffer here (or rotate audio data)

//             outb(nabm + AC97_PCM_OUT_LVI, new_lvi);
//             lvi = new_lvi;
//         }
//     }
// }

// // fills the buffer so it isnt filled with silence
// void fill_ac97_buffers() {
    

//     for (int i = 0; i < BDL_ENTRY_COUNT; i++) {
//         uint16_t* buf = (uint16_t*)buffers[i]->start_addr;

//         for (int j = 0; j < (BUFFER_SIZE / 2); j += 2) {  // 2 bytes per sample, 2 channels
//             if (sample_index < num_samples * 2) {
//                 buf[j] = audio_samples[sample_index++];     // Left
//                 buf[j + 1] = audio_samples[sample_index++]; // Right
// 				//Debug::printf("fill buffer: %d\n", buf[j]);
//             } else {
//                 buf[j] = buf[j + 1] = 0; // Fill silence after end
//             }
//         }
//     }
// }

// // when audio stops we want to interrupt
// void ac97_interrupt_handler() {
//     uint8_t status = inb(nabm + AC97_PCM_OUT_STATUS);

//     if (status & 0x01) { // Buffer completion interrupt
//         reset_buffers();
//     }

//     // Acknowledge the interrupt
//     outb(nabm + AC97_PCM_OUT_STATUS, status);
// }

// // wtf is sample index
// // keep track of index while reading or something so we can use it and refill the buffer???
// // refill buffer once we finish playing the buffers, so it keeps playing

// // void refill_buffers() {
// //     uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
// //     uint8_t next_buffer = (civ + 1) % BDL_ENTRY_COUNT;

// //     uint16_t* buf = (uint16_t*)buffers[next_buffer]->start_addr;

// //     for (int i = 0; i < BUFFER_SIZE / 2; i += 2) { // 2 bytes per sample, 2 channels
// //         if (sample_index < num_samples * 2) {
// //             buf[i] = audio_samples[sample_index++];     // Left
// //             buf[i + 1] = audio_samples[sample_index++]; // Right
// //         } else {
// //             buf[i] = buf[i + 1] = 0; // Fill silence after end
// //         }
// //     }
// // }

// // so it plays silence when a buffer isnt being used
// void reset_buffers() {
//     uint8_t civ = inb(nabm + AC97_PCM_OUT_CIV);
//     uint8_t next_buffer = (civ + 1) % BDL_ENTRY_COUNT;

//     // Fill the next buffer with audio data
//     for (int i = 0; i < BUFFER_SIZE / 4; i++) {
//         buffers[next_buffer]->start_addr[i] = 0; // Example: silence
//     }
// }

// // when we stop doom 
// void shutdown_ac97() {
//     outb(nabm + AC97_PCM_OUT_CONTROL, 0x00); // Stop playback

//     // Free BDL
// }