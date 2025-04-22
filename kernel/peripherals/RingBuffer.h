#pragma once

#include "kernel.h"

template <typename T>
class RingBuffer{
    uint32_t size = 0;
    T* buffer;
    uint32_t first_val = 0;
    uint32_t last_val = 0;
    uint32_t capacity;

public:
    RingBuffer(uint32_t n): capacity(n){
        buffer = new T[capacity];
    }

    ~RingBuffer(){
        delete[] buffer;
    }


    void enqueue(T val){
        // add if full check

        buffer[last_val] = x;
        last_val = (last_val + 1) % capacity;
        size++;
    }

    T dequeue(){
        // add if empty check
        T temp = buffer[first_val];
        first_val = (first_val + 1) % capacity;
        size--;
        return temp;
    }

    T peek(){
        return buffer[first_val];
    }

};