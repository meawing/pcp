#pragma once

#include <optional>
#include <mutex>
#include <condition_variable>

template <class T>
class UnbufferedChannel {
public:
    void Push(T value) {
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
