
#pragma once

#include "semaphore.h"
#include "shared.h"


template <typename T>
class BB {
    T* data;
    uint32_t n;
    Semaphore n_empty;
    Semaphore n_full;
    SpinLock spin{};
    uint32_t front = 0;
    uint32_t back = 0;
public:
    BB(uint32_t n) : data{new T[n]()}, n{n}, n_empty{n}, n_full{0} {}
    ~BB() {
        delete[] data;
    }
    void put(const T& v) {
        n_empty.down();
        spin.lock();
        data[back] = v;
        back = (back+1)%n;
        spin.unlock();
        n_full.up();
    }
    
    //Networking
    bool peekNoWait(T& ret) {
        spin.lock();
        if (n_full.downPossible()) {
            ret = data[front];
            spin.unlock();
            return true;
        } else {
            spin.unlock();
            return false;
        }
    }

    T peek() {
        n_full.down();
        spin.lock();
        auto v = data[front];
        spin.unlock();
        n_full.up();
        return v;
    }


    T get() {
        n_full.down();
        spin.lock();
        auto v = data[front];
        front = (front + 1) % n;
        spin.unlock();
        n_empty.up();
        return v;
    }


        
    //Networking
    bool getNoWait(T& ret) {
        if (!n_full.trydown()) {
            return false;
        }
        spin.lock();
        auto v = data[front];
        ret = v;
        front = (front + 1) % n;
        spin.unlock();
        n_empty.up();
        return true;
    }
};