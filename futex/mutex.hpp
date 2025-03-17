#pragma once

#include <atomic>

#include "futex.hpp"

class Mutex {
public:
    void Lock() {
        // TODO: Your solution
    }

    void Unlock() {
        // TODO: Your solution
    }

private:
    std::atomic<uint32_t> state_{0};
};
