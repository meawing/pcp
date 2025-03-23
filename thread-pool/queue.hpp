#pragma once

#include <queue>
#include <optional>

#include <lines/std/condvar.hpp>
#include <lines/std/mutex.hpp>

template <class T>
class MPMCBlockingUnboundedQueue {
public:
    void Push(T elem) {
        // TODO: Your solution
    }

    std::optional<T> Pop() {
        // TODO: Your solution
    }

    void Close() {
        // TODO: Your solution
    }

private:
    // TODO: Your solution
};
