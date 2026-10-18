#pragma once

#include "debug.h"


// From CS312
template <typename T>
class RingBuffer {
private:
    uint32_t size;
    T* buffer;
    uint32_t first;
    uint32_t last;
    uint32_t capacity;
    mutable SpinLock lock;


public:
    RingBuffer(uint32_t capacity) : size(0), first(0), last(0), capacity(capacity) {
        buffer = new T[capacity];
    }
    
    ~RingBuffer() {
        delete[] buffer;
    }

    // num of things in buffer
    uint32_t getSize() const {
        LockGuard guard{lock};
        return size;
    }
    
    bool isEmpty() const {
        LockGuard guard{lock};
        return size == 0;
    }
    
    bool isFull() const {
        LockGuard guard{lock};
        return size == capacity;
    }
    
    
    // Input producers cannot block in an interrupt. Drop an entire incoming
    // event when the consumer falls behind, preserving queued event boundaries.
    bool enqueue(const T* values, uint32_t count) {
        LockGuard guard{lock};
        if (count > capacity - size) return false;
        for (uint32_t i = 0; i < count; ++i) {
            buffer[last] = values[i];
            last = (last + 1) % capacity;
        }
        size += count;
        return true;
    }

    bool enqueue(T x) {
        return enqueue(&x, 1);
    }
    
    T dequeue() {
        LockGuard guard{lock};
        if (size == 0) {
            Debug::panic("buffer empty\n");}
        T temp = buffer[first];
        first = (first + 1) % capacity;
        size--;
        return temp;
    }
    
    T peek() const {
        LockGuard guard{lock};
        if (size == 0) {
            Debug::panic("buffer empty\n");
        }
        return buffer[first];
    }
    
};
