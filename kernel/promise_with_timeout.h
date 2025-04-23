#pragma once

# include "promise.h"

class PromiseWithTimeout {
    StrongPtr<Promise<char*>> p = StrongPtr<Promise<char*>>::make();
    public:
    PromiseWithTimeout(uint32_t timeout)  {
        thread([p=p, timeout]{
            sleep(timeout);
            p->set(nullptr);
        });
    }

    char* get() {
        return p->get();
    }

    void set(char* value) {
        p->set(value);
    }
};