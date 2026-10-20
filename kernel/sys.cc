    #include <stdint.h>

#include "sys.h"
#include "stdint.h"
#include "debug.h"
#include "idt.h"
#include "machine.h"
#include "ext2.h"
#include "kernel.h"
#include "threads.h"
#include "vme.h"
#include "vmm.h"
#include "libk.h"
#include "elf.h"
#include "promise.h"
#include "networking/socket/socket_sys.h"
#include "vga.h"
#include "../doom-sysroot/include/abi-bits/stat.h"

static constexpr uint32_t GRAPHICS_FILE = 10000;
#include "uio.h" // new code
#include "poll.h"
#include "keyboard.h"
#include "mouse.h"
#include "input.h"
#include "ac97.h"
#include "networking/udp.h"
#include "../include/os/audio.h"
#include "pit.h"
#include "../include/os/runtime.h"

// Graphics buffers must be backed by private process mappings, including
// demand-paged memory. A numeric address check alone accepts unmapped pointers.
static bool graphics_buffer_valid(const void* buffer, uint32_t size) {
    uint32_t address = reinterpret_cast<uint32_t>(buffer);
    if (address < 0x80000000 || address >= 0xF0000000 || size > 0xF0000000 - address) return false;
    auto me = impl::threads::state.current();
    for (uint32_t page = address & ~uint32_t(4095); page < address + size; page += 4096) {
        if (me->vme->get(page) == nullptr) return false;
    }
    return true;
}

static int set_video_palette(const os_video_color* colors, uint32_t first, uint32_t count) {
    if (first >= OS_VIDEO_COLORS || count == 0 || count > OS_VIDEO_COLORS - first) return -1;
    if (!graphics_buffer_valid(colors, count * sizeof(*colors))) return -1;
    os_video_color copy[OS_VIDEO_COLORS];
    memcpy(copy, colors, count * sizeof(*colors));
    vga_set_palette(copy, first, count);
    return 0;
}

    bool address_valid(uint32_t addr, uint32_t size) {
        return addr >= 0x80000000 && size <= 0xFFFFFFFFu - addr;
    }

    uint32_t read(uint32_t fd, void* buf, uint32_t nbyte) {
        if(!address_valid((uint32_t)buf, nbyte) || fd >= PCB_ARR_SIZE) {
            return -1;
        }

        auto me = impl::threads::state.current();
        auto open_file = me->pcb->fdt[fd];
        
        if(open_file == nullptr || open_file->node == nullptr) return -1; // File doesn't exist
        uint32_t bytes_read = open_file->node->read_all(open_file->offset, nbyte, (char*)buf);
        // Debug::printf("RET VAL: %d\n", bytes_read);
        open_file->offset += bytes_read;
        return bytes_read;
    }

    uint32_t write(uint32_t fd, void* buf, int32_t nbyte) {
        if (fd == GRAPHICS_FILE) {
            if (nbyte < 0) return -1;
            if (nbyte > OS_VIDEO_FRAME_BYTES) nbyte = OS_VIDEO_FRAME_BYTES;
            if (!graphics_buffer_valid(buf, nbyte)) return -1;
            memcpy((void*)VGA_ADDRESS, buf, nbyte);
            return nbyte;
        }
        if(nbyte < 0 || !address_valid((uint32_t)buf, nbyte) || fd >= PCB_ARR_SIZE) {
            // Debug::printf("*** bytes_Written from syscall = -1\n");
            return -1;
        }

        auto me = impl::threads::state.current();
        auto open_file = me->pcb->fdt[fd];

        if(open_file == nullptr || open_file->node == nullptr) return -1; // File doesn't exist
        uint32_t bytes_written = open_file->node->write_all(open_file->offset, nbyte, (char*)buf);
        open_file->offset += bytes_written;
        auto data = &open_file->node->data;
        if (open_file->offset > data->size_low) {
            data->size_low = open_file->offset;
        }
        // Debug::printf("*** bytes_Written from syscall = %d\n", bytes_written);
        return bytes_written;
    }

// new code
// write data from multiple buffers to a fd
uint32_t writev(uint32_t fd, const iovec* iov, uint32_t iovcnt) {
    if (fd >= PCB_ARR_SIZE || !address_valid((uint32_t)iov, sizeof(iovec) * iovcnt)) {
        return -1;
    }

    auto me = impl::threads::state.current();
    auto open_file = me->pcb->fdt[fd];
    
    if (open_file == nullptr || open_file->node == nullptr) {
        return -1; // file doesn't exist
    }

    uint32_t total_written = 0;
    for (uint32_t i = 0; i < iovcnt; i++) {
        if (!address_valid((uint32_t)iov[i].iov_base, iov[i].iov_len)) {
            return -1;
        }
        uint32_t bytes_written = open_file->node->write_all(open_file->offset, iov[i].iov_len, (char*)iov[i].iov_base);
        open_file->offset += bytes_written;
        total_written += bytes_written;
        if (bytes_written < iov[i].iov_len) {
            break; // couldn't write all data
        }
    }
    
    auto data = &open_file->node->data;
    if (open_file->offset > data->size_low) {
        data->size_low = open_file->offset;
    }
    
    return total_written;
}

// new code
// system call to make a directory
// do this by creating a new inode with the directory type, and initializing metadata (permissions and . ..)
uint32_t mkdir(const char* path) {
    auto me = impl::threads::state.current();

        StrongPtr<Node> base;
    if (path[0] == '/') {
        base = fs->root;
    } else if (me->pcb->cwd != nullptr) {
        base = me->pcb->cwd;
    } else {
        base = fs->root;
    }

    // Find the last '/' in the path
    int lastSlash = -1;
    for (int i = 0; i < K::strlen(path); i++) {
        if (path[i] == '/') lastSlash = i;
    }

    StrongPtr<Node> parent;
    char* name;
    if (lastSlash != -1) {
        // parentPath = path[0..lastSlash-1]
        char* parentPath = (char*) malloc(lastSlash + 1);
        memcpy(parentPath, path, lastSlash);
        parentPath[lastSlash] = '\0';
        parent = fs->find(base, parentPath);
        // name = path[lastSlash+1..end]
        size_t nameLen = K::strlen(path) - lastSlash - 1;
        name = (char*) malloc(nameLen + 1);
        memcpy(name, path + lastSlash + 1, nameLen);
        name[nameLen] = '\0';
    } else {
        parent = base;
        size_t nameLen = K::strlen(path);
        name = (char*) malloc(nameLen + 1);
        memcpy(name, path, nameLen);
        name[nameLen] = '\0';
    }

    // Validate parent
    if (parent == nullptr || !parent->is_dir()) return -1;
    // Check for existing entry
    if (fs->find(parent, name) != nullptr) return -1;

    auto dirNode = fs->create_inode(parent, name, (4 << 12));
    if (dirNode == nullptr) 
        return -1;
    return 0;
}


// new code
uint32_t ftruncate(uint32_t fd, uint32_t length) {
    if (fd >= PCB_ARR_SIZE) {
        return -1;
    }
    
    auto me = impl::threads::state.current();
    auto open_file = me->pcb->fdt[fd];
    
    if (open_file == nullptr || open_file->node == nullptr) {
        return -1;  // File doesn't exist
    }
    
    if (!open_file->node->is_file()) {
        return -1;  // Not a file
    }
    
    // Truncate the file to the specified length
    open_file->node->truncate(length);
    
    // If the current offset is beyond the new size, adjust it
    if (open_file->offset > length) {
        open_file->offset = length;
    }
    
    return 0;  // Success
}

uint32_t open(const char* fd, int flags, uint16_t mode) {
    if (K::streq(fd, "/dev/vga")) return GRAPHICS_FILE;
    auto me = impl::threads::state.current();
    StrongPtr<Node> checkIn;
    if (K::streq(fd, "/dev/input/keyboard")) {
        for (uint32_t i = 0; i < PCB_ARR_SIZE; i++) {
            if (me->pcb->fdt[i] == nullptr) {
                me->pcb->fdt[i] = StrongPtr<OpenFile> { new OpenFile {StrongPtr<Node> {new Device(&event_buffer)}}};
                return i;
            }
        }
        return -1;
    } else if (K::streq(fd, "/dev/input/mouse")) {
        for (uint32_t i = 0; i < PCB_ARR_SIZE; i++) {
            if (me->pcb->fdt[i] == nullptr) {
                me->pcb->fdt[i] = StrongPtr<OpenFile> { new OpenFile {StrongPtr<Node> {new Device(&buff)}}};
                return i;
            }
        }
    }
    if (fd[0] == '/') {
        checkIn = fs->root;
    }
    else {
        if(me->pcb->cwd == nullptr) checkIn = fs->root;
        else checkIn = me->pcb->cwd;
    }
    StrongPtr<Node> new_node = fs->find(checkIn, fd);
    // check if the creat flag is passed in
    if (new_node == nullptr && (flags & 0x0200)) {;
        int lastSlash = -1;
        for (int i = 0; i < K::strlen(fd); i++) {
            if (fd[i] == '/') lastSlash = i;
        }
        char* name;
        StrongPtr<Node> parent;
        if (lastSlash != -1) {
            char* parentPath = (char*) malloc(lastSlash + 1);
            memcpy(parentPath, fd, lastSlash);
            parentPath[lastSlash] = 0;
            parent = fs->find(checkIn, parentPath);
            name = (char*) malloc(K::strlen(fd) - lastSlash);
            memcpy(name, fd + lastSlash + 1, K::strlen(fd) - lastSlash - 1);
            name[K::strlen(fd) - lastSlash - 1] = 0;
        }
        else {
            parent = checkIn;
            name = (char*) malloc(K::strlen(fd) + 1);
            memcpy(name, fd, K::strlen(fd));
            name[K::strlen(fd)] = 0;
        }
        if (parent == nullptr || !parent->is_dir()) return -1;
        new_node = fs->create_inode(parent, name, mode);
    }

    // If the file was found and O_TRUNC is set, truncate the file
    if (new_node != nullptr && (flags & 0x0400)) {
        if (flags & 0x1 || flags & 0x2) { // check if in write mode (O_WRONLY or O_RDWR)
            new_node->truncate(0); // set the size to 0
        }
    }

    // check if O_EXCL is set, check for existence
    if (new_node != nullptr && (flags & 0x0800)) {
        return -1; // file already exists, cannot create with O_EXCL
    }

    if (new_node == nullptr) return -1;
    StrongPtr<OpenFile> new_open_file = StrongPtr<OpenFile>::make(new_node);
    
    // check if O_APPEND is set
    if (flags & 0x8) {
        new_open_file->offset = new_node->size_in_bytes(); // set offset to end of file
    } else {
        new_open_file->offset = 0; // default to start of file
    }
    
    for(uint32_t i = 0; i < PCB_ARR_SIZE; i++) {
        if(me->pcb->fdt[i] == nullptr) { // Found empty file
            me->pcb->fdt[i] = new_open_file;
            return i;
        }
    }
    Debug::printf("FDT IS FULL!\n");
    return -1;
}

    uint32_t len(uint32_t fd) {
        if(fd >= PCB_ARR_SIZE) {
            return -1;
        }
        auto me = impl::threads::state.current();
        StrongPtr<OpenFile> open_file = me->pcb->fdt[fd];
        if(open_file == nullptr) return -1;
        return open_file->node->size_in_bytes();
    }

    uint32_t close(uint32_t fd) {
        if (fd == GRAPHICS_FILE) return 0;
        auto me = impl::threads::state.current();
        if (fd < PCB_ARR_SIZE) { // file
            me->pcb->fdt[fd] = nullptr;
        } else if (fd < PCB_ARR_SIZE * 2) { // semaphore
            fd -= PCB_ARR_SIZE;
            me->pcb->sdt[fd] = nullptr;
        } else if (fd < PCB_ARR_SIZE * 3) { // children
            fd -= PCB_ARR_SIZE * 2;
            me->pcb->children[fd] = nullptr;
            me->pcb->children_promises[fd] = nullptr;
        } else if (fd < PCB_ARR_SIZE * 4) {
            me->pcb->sockdt[fd - PCB_ARR_SIZE * 3] = nullptr;
        } else { // should never happen
            return -1;
        }

        return 0;
    }

    uint32_t seek(uint32_t fd, uint32_t offset) {
        if(fd >= PCB_ARR_SIZE) {
            return -1;
        }
        auto me = impl::threads::state.current();
        StrongPtr<OpenFile> open_file = me->pcb->fdt[fd];
        if(open_file == nullptr) return -1;
        open_file->offset = offset;
        return open_file->offset;
    }

    uint32_t fork(uint32_t *frame) {
        auto me = impl::threads::state.current();
        
        using namespace impl::threads;
        
        auto parent_pcb = me->pcb;
        auto parent_pd = me->pd;
        auto parent_vme = me->vme;
        auto parent_jiffies = me->at_jiffies;

        uint32_t pc = frame[0];
        uint32_t sp = frame[3];
        
        StrongPtr<PCB> child_pcb = PCB::duplicate(parent_pcb);
        uint32_t child_pd = VMM::new_page_directory();
        VMM::copy_page_directory((uint32_t*)parent_pd, (uint32_t*)child_pd); // PD
        StrongPtr<VME<NoLock>> child_vme = VME<NoLock>::duplicate(me->vme); // VME


        me->pcb->children_promises[child_pcb->pcb_id] = StrongPtr<Promise<int32_t>>::make();
        child_pcb->parent_promise = me->pcb->children_promises[child_pcb->pcb_id];

        // Debug::printf("HERE\n");
        
        thread([parent_jiffies, pc, sp] {
            auto me = impl::threads::state.current();
            me->at_jiffies = parent_jiffies; // Jiffies  
            // Debug::printf("GO TO PC: %x\n", pc);
            switchToUser(pc, sp, 0);

        }, child_pd, child_pcb, child_vme);
        // Debug::printf("%s\n", new_tcb->pcb->pcb_id);

        return child_pcb->pcb_id + PCB_ARR_SIZE * 2;
    }

    uint32_t execl(const char* path, const char* argv[]) {
        auto me = impl::threads::state.current();
        
        StrongPtr<Node> node = me->pcb->cwd;
        if(node == nullptr) {
            me->pcb->cwd = node;
            node = fs->root;
        }
        StrongPtr<Node> new_program = fs->find(node, path);
        if(new_program == nullptr) {
            Debug::printf("INVALID PROGRAM PATH\n");
            return -1;
        }
        
        // COPY ARGS TO KERNEL
        uint32_t argc = 0;
        while(argv[argc] != 0) {
            if((uint32_t)argv[argc] < 0x80000000 || (uint32_t)argv[argc] + K::strlen(argv[argc]) >= 0xF0000000) {
                Debug::printf("INVALID ARGUMENT\n");
                return -1;
            }
            argc++;
        }

        const char* k_argv[argc + 1];
        for(uint32_t i = 0; i < argc; i++) {
            uint32_t length = K::strlen(argv[i]) + 1;
            k_argv[i] = new char[length];
            memcpy((void*)k_argv[i], argv[i], length);
        }
        k_argv[argc] = nullptr;
        
        // CLEAR ADDR SPACE
        me->vme = nullptr;
        for(uint32_t i = (0x80000000 >> 22); i < (0xF0000000 >> 22); i++) {
            VMM::remove_PT_mapping((uint32_t*)me->pd, i);
        }
        
        me->vme = StrongPtr<VME<NoLock>>::make(0x80000000, 0xF0000000);
        
        uint32_t e = ELF::load(new_program);

        const uint32_t stack_size = 134217728;
        void* stack_start = VMM::naive_mmap(stack_size, false, {}, 0, false);
        uint32_t userEsp = ((uint32_t)stack_start + stack_size - 16);
        
        userEsp = (uint32_t)load_args((uint32_t*)userEsp, k_argv);
        
        switchToUser(e,userEsp,0);
        return -1;
    }

    void exit(uint32_t rc) {
        auto me = impl::threads::state.current();
        if(me->pcb->parent_promise != nullptr) {
            me->pcb->parent_promise->set(rc);
        }
        stop();
    }

    uint32_t wait(uint32_t id, uint32_t* status) {
        //! Need to implement new code for wait for any child
        auto me = impl::threads::state.current();
        if(id >= PCB_ARR_SIZE || me->pcb->children_promises[id] == nullptr) {
            return -1;
        }
        *status = me->pcb->children_promises[id]->get();
        me->pcb->children_promises[id] = nullptr;
        me->pcb->children[id] = nullptr;

        return 0;
    }

    uint32_t chdir(const char* path) {
        auto me = impl::threads::state.current();
        ASSERT(!(fs == nullptr));
        ASSERT(!(me == nullptr));
        ASSERT(!(me->pcb == nullptr));

        StrongPtr<Node> node = me->pcb->cwd;
        if(node == nullptr) node = fs->root;

        auto new_node = fs->find(node, path);
        if(new_node == nullptr) return -1;
        me->pcb->cwd = new_node;
        return 0;
    }

    uint32_t sem(uint32_t initial) {
        auto me = impl::threads::state.current();
        uint32_t id = 0;
        while(!(me->pcb->sdt[id] == nullptr) && id < PCB_ARR_SIZE) {
            id++;
        }
        if(id == PCB_ARR_SIZE) return -1;

        me->pcb->sdt[id] = StrongPtr<Semaphore>::make(initial);
        return id + PCB_ARR_SIZE;
    }

    uint32_t up(uint32_t id) {
        auto me = impl::threads::state.current();
        if(id < 0 || id >= PCB_ARR_SIZE || me->pcb->sdt[id] == nullptr ) {
            Debug::printf("UP IS INVALID\n");
            return -1;
        }
        me->pcb->sdt[id]->up();

        return 0;
    }
    uint32_t down(uint32_t id) {
        auto me = impl::threads::state.current();
        if(id < 0 || id >= PCB_ARR_SIZE || me->pcb->sdt[id] == nullptr ) {
            Debug::printf("DOWN IS INVALID\n");
            return -1;
        }
        me->pcb->sdt[id]->down();
        
        return 0;
    }

    void* naive_mmap(uint32_t size, bool is_shared, uint32_t file, uint32_t offset) {
        auto me = impl::threads::state.current();
        StrongPtr<OpenFile> open_file = me->pcb->fdt[file];
        StrongPtr<Node> node = (open_file == nullptr) ? StrongPtr<Node>() : open_file->node;

        return VMM::naive_mmap(size, is_shared, node, offset, true);
    }

    void naive_unmap(void* p) {
        VMM::naive_munmap(p, true);
    }

uint32_t print_dir() {
    auto me = impl::threads::state.current();
    if (me->pcb->cwd == nullptr) me->pcb->cwd = fs->root;
    me->pcb->cwd->entries([](uint32_t inode, char *name, uint8_t name_length){
        if (!(K::streq(name, ".") || K::streq(name, "..") || K::streq(name, "lost+found"))) {
            Debug::printf("%s\t", name);
        }
        return 0;
    });
    Debug::printf("\n");
    return 0;
}

extern "C" int sysHandler(uint32_t eax, uint32_t *frame) {
    //Debug::printf("*** syscall #%d\n",eax);
    uint32_t *user_sp = (uint32_t*)frame[3];
    if((uint32_t)user_sp < 0x80000000 || (uint32_t)user_sp >= 0xF0000000) {
        Debug::printf("USER ESP IS INVALID\n");
        exit(-1);
    }
    // The historical socket ABI is incomplete; use the bounded native UDP ABI.
    if (eax >= 200 && eax < 220) return -1;
    switch (eax) {
    case OS_SYS_NET_CONFIGURE: return NativeNet::configure(user_sp[1]);
    case OS_SYS_UDP_OPEN: return NativeNet::open(user_sp[1]);
    case OS_SYS_UDP_CLOSE: return NativeNet::close(user_sp[1]);
    case OS_SYS_UDP_SEND:
    case OS_SYS_UDP_RECV: {
        auto packet = reinterpret_cast<os_datagram*>(user_sp[2]);
        if (!graphics_buffer_valid(packet, sizeof(*packet))) return -1;
        return eax == OS_SYS_UDP_SEND ? NativeNet::send(user_sp[1], packet) : NativeNet::receive(user_sp[1], packet);
    }
    case OS_SYS_NET_STATS: {
        auto out = reinterpret_cast<os_net_stats*>(user_sp[1]);
        return graphics_buffer_valid(out, sizeof(*out)) ? NativeNet::stats(out) : -1;
    }
    case OS_SYS_AUDIO_OPEN:
        return Ac97::open();
    case OS_SYS_AUDIO_WRITE: {
        uint32_t count = user_sp[2];
        auto pcm = reinterpret_cast<const int16_t*>(user_sp[1]);
        if (count == 0 || count > OS_AUDIO_MAX_FRAMES ||
            !graphics_buffer_valid(pcm, count * OS_AUDIO_CHANNELS * sizeof(int16_t))) return -1;
        return Ac97::write(pcm, count);
    }
    case OS_SYS_AUDIO_QUEUED:
        return Ac97::queued();
    case OS_SYS_AUDIO_CLOSE:
        return Ac97::close();
    case OS_SYS_INPUT_READ: {
        auto out = reinterpret_cast<os_input_frame*>(user_sp[1]);
        if (!graphics_buffer_valid(out, sizeof(*out))) return -1;
        os_input_frame frame;
        Input::read(frame);
        // Copy after releasing the input lock: user pages can fault/block.
        *out = frame;
        return 0;
    }
    case OS_SYS_MILLISECONDS:
        return Pit::milliseconds();
    case OS_SYS_YIELD:
        yield();
        return 0;
    case OS_SYS_ALLOCATE:
        if (!user_sp[1] || user_sp[1] > 64 * 1024 * 1024) return -1;
        return uint32_t(VMM::naive_mmap(user_sp[1], false, {}, 0, true));
    case OS_SYS_FILE_INFO: {
        unsigned fd = user_sp[1];
        auto out = reinterpret_cast<os_file_info*>(user_sp[2]);
        if (fd >= PCB_ARR_SIZE || !graphics_buffer_valid(out, sizeof(*out))) return -1;
        auto file = impl::threads::state.current()->pcb->fdt[fd];
        if (file == nullptr || file->node == nullptr) return -1;
        auto node = file->node;
        *out = {node->data.mode, node->size_in_bytes(), node->number,
                node->data.n_links, node->data.n_sectors, node->block_size};
        return 0;
    }
    case OS_SYS_FILE_SEEK: {
        unsigned fd = user_sp[1], whence = user_sp[3];
        if (fd >= PCB_ARR_SIZE || whence > 2) return -1;
        auto file = impl::threads::state.current()->pcb->fdt[fd];
        if (file == nullptr || file->node == nullptr) return -1;
        int64_t offset = int32_t(user_sp[2]);
        if (whence == 1) offset += file->offset;
        if (whence == 2) offset += file->node->size_in_bytes();
        if (offset < 0 || offset > 0x7fffffff) return -1;
        return file->offset = uint32_t(offset);
    }
    case 0:
        exit(user_sp[1]);    
        return -1;
    case 1:
        return write(user_sp[1], (void*)user_sp[2], user_sp[3]);
    case 2:
        return fork(frame);
    case 3:
        return sem(user_sp[1]);
    case 4:
        return up(user_sp[1] - PCB_ARR_SIZE);
    case 5:
        return down(user_sp[1] - PCB_ARR_SIZE);
    case 6:
        return close(user_sp[1]);
    case 7: /* shutdown */
        fs->buffer_cache->flush();
        Debug::shutdown();
        return -1;
    case 8:
        return wait(user_sp[1] - PCB_ARR_SIZE * 2, (uint32_t*)user_sp[2]);
    case 9:
        return execl((const char*)user_sp[1], (const char**)((char*)user_sp + 8));
    case 10:
        return open((const char*)user_sp[1], (int) user_sp[2], (uint16_t) user_sp[3]);
    case 11:
        return len(user_sp[1]);
    case 12:
        return read(user_sp[1], (void*)user_sp[2], user_sp[3]);
    case 13:
        return seek(user_sp[1], user_sp[2]);
    case 14: // new code
        return writev(user_sp[1], (iovec*)user_sp[2], user_sp[3]);
    case 15: // new code
        return mkdir((const char*)user_sp[1]);
    case 16: // new code
        return ftruncate(user_sp[1], user_sp[2]);
    case 90:{
        auto me = impl::threads::state.current();
        StrongPtr<OpenFile> open_file = me->pcb->fdt[user_sp[5]];
        StrongPtr<Node> node = (open_file == nullptr) ? StrongPtr<Node>() : open_file->node;
        return (uint32_t)VMM::mmap1((uint32_t* )user_sp[1], user_sp[2], user_sp[3],user_sp[4], node ,user_sp[6]);
    }
    case 91:
        VMM::munmap((void*) user_sp[1], user_sp[2], user_sp[3]);
        return 0;
    case OS_SYS_VIDEO_INIT:
        vga_init();
        return 0;
    case OS_SYS_VIDEO_PALETTE:
        return set_video_palette((const os_video_color*)user_sp[1], user_sp[2], user_sp[3]);
    case 250:
        return stat((const char*)user_sp[1], (struct stat*)user_sp[2]);
    case 251:
        return fstat(user_sp[1], (struct stat*)user_sp[2]);
    case 100:
        return chdir((const char*)user_sp[1]);
    case 101:
        return (uint32_t)naive_mmap(user_sp[1], user_sp[2], user_sp[3], user_sp[4]);
    case 102:
        naive_unmap((void*)user_sp[1]);
        return 0;
    
    case 104:
        return (uint32_t) VMM::map_vga();

    
    case 418:
        Debug::printf("*** I'm a teapot\n");
        return -1;
    case 1970:
        return print_dir();
    case 499:
        return Poll::poll((Poll::Pollfd*)user_sp[1], (int)user_sp[2], (int)user_sp[3], impl::threads::state.current()->pcb.operator->());
    default:
        Debug::printf("*** unknown system call %d\n",eax);
        return -1;
    }
    
}   

    void SYS::init(void) {
        IDT::trap(48,(uint32_t)sysHandler_,3);
    }

// Keep libc metadata calls outside the 200-249 socket range.
static int fill_stat(StrongPtr<Node> node, struct stat* buf) {
    if (node == nullptr || !address_valid((uint32_t)buf, sizeof(*buf))) return -1;
    bzero(buf, sizeof(*buf));
    buf->st_mode = node->data.mode;
    buf->st_ino = node->number;
    buf->__st_ino_truncated = node->number;
    buf->st_nlink = node->data.n_links;
    buf->st_uid = node->data.uid;
    buf->st_gid = node->data.gid;
    buf->st_size = node->data.size_low;
    buf->st_blksize = node->block_size;
    buf->st_blocks = node->data.n_sectors;
    buf->st_atim.tv_sec = buf->__st_atim32.tv_sec = node->data.atime;
    buf->st_mtim.tv_sec = buf->__st_mtim32.tv_sec = node->data.mtime;
    buf->st_ctim.tv_sec = buf->__st_ctim32.tv_sec = node->data.ctime;
    return 0;
}
int stat(const char* path, struct stat* buf) {
    auto me = impl::threads::state.current();
    auto base = me->pcb->cwd == nullptr ? fs->root : me->pcb->cwd;
    return fill_stat(fs->find(base, path), buf);
}
int fstat(int fd, struct stat* buf) {
    if (fd < 0 || static_cast<uint32_t>(fd) >= PCB_ARR_SIZE) return -1;
    auto file = impl::threads::state.current()->pcb->fdt[fd];
    if (file == nullptr) return -1;
    return fill_stat(file->node, buf);
}
