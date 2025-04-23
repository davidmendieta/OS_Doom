#pragma once

#include "debug.h"
#include "semaphore.h"
#include "atomic.h"
template <typename T>
class Promise {
    Semaphore sem{0};
    T value{};
    Atomic<bool> ready{false};
public:
    void set(T const& v) {
        if (ready.exchange(true)) {
           return; // already set
        }
        value = v;
        sem.up();
    }
    T get() {
        sem.down();
        sem.up();
        return value;
    }
};



