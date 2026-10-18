#pragma once

#include "blocking_lock.h"
#include "ide.h"
#include "block_io.h"
#include "shared.h"
#include "libk.h"

namespace impl::ext2 {

// Direct-mapped, bounded sector cache. A single blocking lock keeps replacement,
// copying and writeback atomic, including when an IDE operation yields the CPU.
// The old bucket-level LRU could link its head to itself and lose its tail.
class BufferCache: public BlockIO {
    struct Entry {
        uint32_t sector = 0;
        bool valid = false;
        bool dirty = false;
        char* bytes = nullptr;
    };
    static constexpr uint32_t capacity = 997;
    StrongPtr<Ide> ide;
    Entry entries[capacity]{};
    BlockingLock lock{};

    void writeback(Entry& entry) {
        if (entry.valid && entry.dirty) {
            ide->write_block(entry.sector, entry.bytes);
            entry.dirty = false;
        }
    }
    Entry& get(uint32_t sector, bool read_existing) {
        Entry& entry = entries[sector % capacity];
        if (!entry.valid || entry.sector != sector) {
            writeback(entry);
            if (!entry.bytes) entry.bytes = new char[block_size];
            if (read_existing) ide->read_block(sector, entry.bytes);
            entry.sector = sector;
            entry.valid = true;
        }
        return entry;
    }

public:
    explicit BufferCache(StrongPtr<Ide> disk): BlockIO(disk->block_size), ide(disk) {}
    virtual ~BufferCache() {
        flush();
        for (auto& entry : entries) delete[] entry.bytes;
    }
    uint32_t size_in_bytes() override { return ide->size_in_bytes(); }
    void flush() {
        LockGuard<BlockingLock> guard{lock};
        for (auto& entry : entries) writeback(entry);
    }
    void read_block(uint32_t sector, char* out) override {
        LockGuard<BlockingLock> guard{lock};
        auto& entry = get(sector, true);
        memcpy(out, entry.bytes, block_size);
    }
    void write_block(uint32_t sector, const char* data) override {
        LockGuard<BlockingLock> guard{lock};
        auto& entry = get(sector, false);
        memcpy(entry.bytes, data, block_size);
        entry.dirty = true;
    }
};
}
