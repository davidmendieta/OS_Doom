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


public:
    RingBuffer(uint32_t capacity) : size(0), first(0), last(0), capacity(capacity) {
        buffer = new T[capacity];
    }
    
    ~RingBuffer() {
        delete[] buffer;
    }

    // num of things in buffer
    uint32_t getSize() const {
        return size;
    }
    
    bool isEmpty() const {
        return size == 0;
    }
    
    bool isFull() const {
        return size == capacity;
    }
    
    
    void enqueue(T x) {
        if (isFull()) {
            Debug::panic("buffer full\n");
        }
        buffer[last] = x;
        last = (last + 1) % capacity;
        size++;
    }
    
    T dequeue() {
        if (isEmpty()) {
            Debug::panic("buffer empty\n");}
        T temp = buffer[first];
        first = (first + 1) % capacity;
        size--;
        return temp;
    }
    
    T peek() const {
        if (isEmpty()) {
            Debug::panic("buffer empty\n");
        }
        return buffer[first];
    }
    
};