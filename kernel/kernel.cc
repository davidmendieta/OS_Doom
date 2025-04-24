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

// ── WAV PLAYBACK TEST ────────────────────────────────────────────────────

auto d = StrongPtr<Ide>::make(1,3);
fs = StrongPtr<Ext2>::make(d);

Debug::printf("*** launching WAV audio test\n");

// 1) find the file

Debug::printf("*** root: %x\n", fs->root);
// StrongPtr<Node> node = fs->find(fs->root, "./testcases/audio/test.wav");
// StrongPtr<Node> node = fs->find(fs->root, "./kernel/audio/audio.cc");
StrongPtr<Node> node = fs->find(fs->root, "./test.wav");


Debug::printf("*** erm\n");
if (node == nullptr) {
    Debug::printf("*** WAV: file not found\n");
} else {
    // 2) mmap + read header
    uint8_t *map = (uint8_t*)VMM::naive_mmap(4096*6, true, {}, 0, false);
    int64_t got = node->read_all(0, 4096*6, (char*)map);
    if (got < 0) {
        Debug::printf("*** WAV: read_all failed (%d)\n", got);
        VMM::naive_munmap(map, true);
    } else {
        // 3) parse the header
        WavFmtHeader *hdr = (WavFmtHeader*)map;
        bool riffOK = hdr->riff_id[0]=='R' &&
                      hdr->riff_id[1]=='I' &&
                      hdr->riff_id[2]=='F' &&
                      hdr->riff_id[3]=='F';
        bool waveOK = hdr->wave_id[0]=='W' &&
                      hdr->wave_id[1]=='A' &&
                      hdr->wave_id[2]=='V' &&
                      hdr->wave_id[3]=='E';
        bool fmtOK  = hdr->fmt_id[0]=='f' &&
                      hdr->fmt_id[1]=='m' &&
                      hdr->fmt_id[2]=='t' &&
                      hdr->fmt_id[3]==' ';
        if (!riffOK || !waveOK || !fmtOK || hdr->audio_fmt != 1) {
            Debug::printf("*** WAV: unsupported format\n");
        } else {


            // 4) find "data" chunk
            uint8_t *ptr = map + 12 + 8 + hdr->fmt_sz;
            uint32_t data_sz = 0;
            while (true) {
                char id0 = ptr[0], id1 = ptr[1], id2 = ptr[2], id3 = ptr[3];
                ptr += 4;
                uint32_t sz = *(uint32_t*)ptr;
                ptr += 4;
                if (id0=='d' && id1=='a' && id2=='t' && id3=='a') {
                    data_sz = sz;
                    break;
                }
                ptr += sz;

            }


            // 5) copy PCM samples

            uint16_t *pcm = (uint16_t*)ptr;
            uint32_t maxS = SAMPLE_RATE * 2;
            uint32_t nS   = data_sz / (hdr->bits_per_sample/8);
            num_samples   = (nS > maxS ? maxS : nS);
                Debug::printf("*** num samples %u", num_samples);
            for (uint32_t i = 0; i < 4096*2; i++) {
                audio_samples[i] = pcm[i];
                Debug::printf("*** WAV:  %i\n", i);
            }

            Debug::printf("*** WAV: loaded %u samples\n", num_samples);

            // 6) play
            if (init_ac97() == 0) {
                Debug::printf("*** audio init success\n");
                poll_ac97_loop();  // never returns
            } else {
                Debug::printf("*** audio init failed\n");
            }
            shutdown_ac97();
            Debug::printf("*** audio shutdown success\n");
        }

        VMM::naive_munmap(map, true);
    }
}
// ──────────────────────────────────────────────────────────────────────────


}

