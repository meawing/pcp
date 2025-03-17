#pragma once

#include <atomic>
#include <cstddef>
#include <vector>
#include <optional>

template <class T>
class SPSCQueue {
    struct Slot {
        T value;
    };

public:
    // TODO: Your solution

    bool Push(T value) {
        // TODO: Your solution
    }

    std::optional<T> Pop() {
        // TODO: Your solution
    }

private:
    // TODO: Your solution
};
