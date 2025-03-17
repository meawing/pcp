#pragma once

#include <atomic>

#include "futex.hpp"

class Condvar {
public:
    template <class Lockable>
    void Wait(Lockable& lock) {
        // TODO: Your solution
    }

    void NotifyOne() {
        // TODO: Your solution
    }

    void NotifyAll() {
        // TODO: Your solution
    }

private:
    std::atomic<uint32_t> counter_{0};
    std::atomic<uint32_t> waiters_{0};
};
