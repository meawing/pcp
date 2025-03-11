#pragma once

#include <atomic>

struct SpinLock {
    void Lock();
    void Unlock();

    void lock();    // NOLINT
    void unlock();  // NOLINT

    // TODO: Your solution
};
