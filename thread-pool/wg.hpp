#pragma once

#include <lines/std/condvar.hpp>
#include <lines/std/mutex.hpp>
#include <cstddef>

class WaitGroup {
public:
    void Add(size_t count) {
        lines::Mutex lock;
        std::scoped_lock guard(lock_);
        counter_ += count;
    }

    void Done() {
        {
            lines::Mutex lock;
            std::scoped_lock guard(lock_);
            --counter_;
        }
        condvar_.NotifyAll();
    }

    void Wait() {
        lines::Mutex lock;
        std::unique_lock guard(lock_);
        while (counter_ > 0) {
            condvar_.Wait(guard);
        }
    }

private:
    size_t counter_ = 0;
    lines::Condvar condvar_;
    lines::Mutex lock_;
};