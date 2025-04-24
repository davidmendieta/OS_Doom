#include "debug.h"
#include "ide.h"
#include "ext2.h"
#include "elf.h"
#include "machine.h"
#include "libk.h"
#include "config.h"
#include "shared.h"
#include "ac97.h"
#include "libk.h"
#include "audio_data.h"

#define SAMPLE_RATE 48000

StrongPtr<Ext2> fs{};

// at top of kernel.cc, before kernelMain
struct WavFmtHeader {
    char     riff_id[4];
    uint32_t riff_sz;
    char     wave_id[4];
    char     fmt_id[4];
    uint32_t fmt_sz;
    uint16_t audio_fmt;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

void generate_test_tone() {
    Debug::printf("*** Generating test sine wave at 440Hz\n");
    
    // Reset global variables
    num_samples = SAMPLE_RATE; // one second of audio
    sample_index = 0;
    
    // Pre-computed sine wave table (one full cycle)
    // This represents 32 points of one sine wave cycle with values scaled to 16-bit range
    const int16_t sine_table[32] = {
        0, 6392, 12539, 18204, 23169, 27244, 30272, 32137,
        32767, 32137, 30272, 27244, 23169, 18204, 12539, 6392,
        0, -6392, -12539, -18204, -23169, -27244, -30272, -32137,
        -32767, -32137, -30272, -27244, -23169, -18204, -12539, -6392
    };
    
    // Generate a 440Hz sine wave
    // 440Hz means 440 cycles per second
    // At 48000 samples per second, that's about 109 samples per cycle
    const uint32_t samples_per_cycle = SAMPLE_RATE / 440;
    const int16_t amplitude = 16384; // Half of max 16-bit amplitude
    
    for (uint32_t i = 0; i < num_samples; i++) {
        // Calculate position in sine wave cycle (0-31)
        uint32_t position = (i * 32) / samples_per_cycle % 32;
        
        // Get value from sine table
        int16_t sample_value = (sine_table[position] * amplitude) / 32767;
        
        // Stereo output (same for both channels)
        audio_samples[i*2] = sample_value;     // Left
        audio_samples[i*2+1] = sample_value;   // Right
    }
    
    Debug::printf("*** Generated %u samples of sine wave\n", num_samples*2);
}

uint32_t* load_args(uint32_t* userEsp, const char** argv) {
    uint32_t argc = 0;
    while(argv[argc] != nullptr) {
        argc++;
    }
    
    uint32_t length[argc];
    uint32_t sum = 0;
    for(uint32_t i = 0; i < argc; i++) {
        length[i] = K::strlen(argv[i]) + 1;
        sum += length[i];
    }

    uint32_t next = sizeof(char*) * (argc + 1 + 1); // account for argc, argv
    userEsp -= ((sum + next + 15) / 16) * 16;

    ((uint32_t*)userEsp)[0] = argc; // adds argc to stack
    ((uint32_t*)userEsp)[1] = (uint32_t)userEsp + 8; // makes argv point to the next slot
    for(uint32_t i = 0; i < argc; i++) {
        ((uint32_t*)userEsp)[1 + 1 + i] = (uint32_t)userEsp + next;
        uint32_t len = length[i];
        memcpy((void*)((char*)userEsp + next), argv[i], len);
        next += len;
    }

    return userEsp;
}

void kernelMain(void) {
    /*
	Debug::printf("im here in kernel main\n");
    Debug::printf("f sdfkj sadhfkjdash flkjsadhfkljsdah flkjsdahf lkajds \n");

    auto d = StrongPtr<Ide>::make(1,0);
    fs = StrongPtr<Ext2>::make(d);
    auto init = fs->find(fs->root, "/sbin/init");
	Debug::printf("im here in kernel main 2\n");
    // Debug::printf("loading init\n");
    uint32_t e = ELF::load(init);
    Debug::printf("entry %x\n",e);
    const uint32_t stack_size = 134217728;
    void* stack_start = VMM::naive_mmap(stack_size, false, {}, 0, false);
    uint32_t userEsp = ((uint32_t)stack_start + stack_size - 16);
    Debug::printf("im here in kernel main 3\n");
    const char* argv[] = { "init", nullptr };
    userEsp = (uint32_t)load_args((uint32_t*)userEsp, argv);


	Debug::printf("*** launching AC97 audio test\n");

    if (init_ac97() == 0) {
        Debug::printf("*** audio init success\n");
    } else {
        Debug::printf("*** audio init failed\n");
    }
	poll_ac97_loop();   
    // Wait a bit to give QEMU time to emit sound
    for (int i = 0; i < 100; i++) {
		Debug::printf("i %d\n", i);
	}

    shutdown_ac97();
    Debug::printf("*** audio shutdown success\n");

    
    // userEsp-=16; // Account for argc and argv
    
    // ((uint32_t*)userEsp)[0] = 1;
    // ((uint32_t*)userEsp)[1] = (uint32_t)userEsp + 8;
    // ((uint32_t*)userEsp)[2] = (uint32_t)userEsp + 12;

    // ((char*)userEsp)[12] = 'i';
    // ((char*)userEsp)[13] = 'n';
    // ((char*)userEsp)[14] = 'i';
    // ((char*)userEsp)[15] = 't';
    // ((char*)userEsp)[16] = '\0';

    // char** argv = (char**)((uint32_t*)userEsp)[1];

    // Debug::printf("HERE %s\n", argv[0]);



    ASSERT(userEsp % 16 == 0);

    // Debug::printf("user esp %x\n",userEsp);
    // Current state:
    //     - %eip points somewhere in the middle of kernelMain
    //     - %cs contains kernelCS (CPL = 0)
    //     - %esp points in the middle of the thread's kernel stack
    //     - %ss contains kernelSS
    //     - %eflags has IF=1
    // Desired state:
    //     - %eip points at e
    //     - %cs contains userCS (CPL = 3)
    //     - %eflags continues to have IF=1
    //     - %esp points to the bottom of the user stack
    //     - %ss contain userSS
    // User mode will never "return" from switchToUser. It will
    // enter the kernel through interrupts, exceptions, and system
    // calls.
    switchToUser(e,userEsp,0);
    Debug::panic("*** implement switchToUser in machine.S\n");
    */

// In kernel.cc, replace the WAV playback test section with this improved version

// ── WAV PLAYBACK TEST ────────────────────────────────────────────────────

// auto d = StrongPtr<Ide>::make(1,3);
// fs = StrongPtr<Ext2>::make(d);

// Debug::printf("*** launching WAV audio test\n");

// // 1) find the file
// Debug::printf("*** root: %x\n", fs->root);
// StrongPtr<Node> node = fs->find(fs->root, "./test.wav");

// Debug::printf("*** looking for WAV file\n");
// if (node == nullptr) {
//     Debug::printf("*** WAV: file not found\n");
// } else {
//     // 2) mmap + read header
//     uint64_t file_size = node->size_in_bytes();
//     uint8_t *map = (uint8_t*)VMM::naive_mmap(file_size, true, {}, 0, false);
    
//     if (!map) {
//         Debug::printf("*** WAV: failed to allocate memory for file\n");
//         return;
//     }
    
//     int64_t got = node->read_all(0, file_size, (char*)map);
//     if (got < 0) {
//         Debug::printf("*** WAV: read_all failed (%d)\n", got);
//         VMM::naive_munmap(map, true);
//     } else {
//         Debug::printf("*** WAV: loaded %d bytes from file\n", got);
        
//         // 3) parse the header
//         WavFmtHeader *hdr = (WavFmtHeader*)map;
//         bool riffOK = hdr->riff_id[0]=='R' &&
//                       hdr->riff_id[1]=='I' &&
//                       hdr->riff_id[2]=='F' &&
//                       hdr->riff_id[3]=='F';
//         bool waveOK = hdr->wave_id[0]=='W' &&
//                       hdr->wave_id[1]=='A' &&
//                       hdr->wave_id[2]=='V' &&
//                       hdr->wave_id[3]=='E';
//         bool fmtOK  = hdr->fmt_id[0]=='f' &&
//                       hdr->fmt_id[1]=='m' &&
//                       hdr->fmt_id[2]=='t' &&
//                       hdr->fmt_id[3]==' ';
                      
//         Debug::printf("*** WAV: Format - Channels: %d, Sample Rate: %d, Bits: %d\n", 
//             hdr->channels, hdr->sample_rate, hdr->bits_per_sample);
            
//         if (!riffOK || !waveOK || !fmtOK) {
//             Debug::printf("*** WAV: not a valid WAV file\n");
//         } else if (hdr->audio_fmt != 1) {
//             Debug::printf("*** WAV: unsupported format (not PCM)\n");
//         } else if (hdr->bits_per_sample != 16) {
//             Debug::printf("*** WAV: unsupported bits per sample (need 16-bit)\n");
//         } else {
//             // 4) find "data" chunk
//             uint8_t *ptr = map + 12 + 8 + hdr->fmt_sz;
//             uint32_t data_sz = 0;
//             bool found_data = false;
            
//             // Search for data chunk (with safety limit to prevent infinite loop)
//             uint32_t search_limit = 1024; // Max bytes to search for data chunk
//             uint32_t bytes_searched = 0;
            
//             while (bytes_searched < search_limit && ptr < map + file_size - 8) {
//                 char id0 = ptr[0], id1 = ptr[1], id2 = ptr[2], id3 = ptr[3];
//                 ptr += 4;
//                 uint32_t sz = *(uint32_t*)ptr;
//                 ptr += 4;
//                 bytes_searched += 8;
                
//                 Debug::printf("*** WAV: Found chunk '%.4s' size %d\n", ptr-8, sz);
                
//                 if (id0=='d' && id1=='a' && id2=='t' && id3=='a') {
//                     data_sz = sz;
//                     found_data = true;
//                     break;
//                 }
                
//                 // Safety check to prevent overflow
//                 if (ptr + sz > map + file_size) {
//                     Debug::printf("*** WAV: Invalid chunk size\n");
//                     break;
//                 }
                
//                 ptr += sz;
//                 bytes_searched += sz;
//             }
            
//             if (!found_data) {
//                 Debug::printf("*** WAV: No data chunk found\n");
//             } else {
//                 Debug::printf("*** WAV: Found data chunk, size: %d bytes\n", data_sz);
                
//                 // 5) Calculate number of samples and perform conversion if needed
//                 uint32_t sample_size = hdr->bits_per_sample / 8;
//                 uint32_t frame_size = sample_size * hdr->channels;
//                 uint32_t total_frames = data_sz / frame_size;
                
//                 Debug::printf("*** WAV: Total frames: %d\n", total_frames);
                
//                 // Limit to our buffer size
//                 // uint32_t maxS = SAMPLE_RATE * 2; // Stereo, so 2 samples per frame
//                 uint32_t frames_to_copy = (total_frames > SAMPLE_RATE) ? SAMPLE_RATE : total_frames;
                
//                 // Reset global variables
//                 num_samples = frames_to_copy;
//                 sample_index = 0;
                
//                 Debug::printf("*** WAV: Converting %d frames to AC97 format\n", frames_to_copy);
                
//                 // Convert samples based on input format
//                 int16_t *pcm = (int16_t*)ptr;
                
//                 if (hdr->channels == 1) {
//                     // Mono to stereo conversion
//                     for (uint32_t i = 0; i < frames_to_copy; i++) {
//                         int16_t sample = pcm[i];
//                         // For mono, duplicate the sample to both channels
//                         audio_samples[i*2] = sample;     // Left
//                         audio_samples[i*2+1] = sample;   // Right
//                     }
//                     num_samples = frames_to_copy;
//                 } else if (hdr->channels == 2) {
//                     // Already stereo, just copy
//                     for (uint32_t i = 0; i < frames_to_copy * 2; i++) {
//                         audio_samples[i] = pcm[i];
//                     }
//                     num_samples = frames_to_copy;
//                 } else {
//                     // More than 2 channels, take just the first two
//                     for (uint32_t i = 0; i < frames_to_copy; i++) {
//                         audio_samples[i*2] = pcm[i*hdr->channels];       // Left (first channel)
//                         audio_samples[i*2+1] = pcm[i*hdr->channels+1];   // Right (second channel)
//                     }
//                     num_samples = frames_to_copy;
//                 }
                
//                 // Handle sample rate conversion if needed
//                 if (hdr->sample_rate != SAMPLE_RATE) {
//                     Debug::printf("*** WAV: Warning - sample rate mismatch (%d vs %d)\n", 
//                                  hdr->sample_rate, SAMPLE_RATE);
//                     // Simple sample rate conversion not implemented
//                     // Just a warning for now, but this would cause speed/pitch issues
//                 }
                
//                 Debug::printf("*** WAV: Loaded %u samples (%u frames)\n", num_samples*2, num_samples);
                
//                 // 6) Initialize and play
//                 if (init_ac97() == 0) {
//                     Debug::printf("*** audio init success\n");
//                     poll_ac97_loop();  // never returns
//                 } else {
//                     Debug::printf("*** audio init failed\n");
//                 }
//                 shutdown_ac97();
//                 Debug::printf("*** audio shutdown success\n");
//             }
//         }
//         VMM::naive_munmap(map, true);
//     }
// }
// ──────────────────────────────────────────────────────────────────────────
generate_test_tone();

// Initialize and play
if (init_ac97() == 0) {
    Debug::printf("*** audio init success\n");
    poll_ac97_loop();  // never returns
} else {
    Debug::printf("*** audio init failed\n");
}
shutdown_ac97();
Debug::printf("*** audio shutdown success\n");

}

