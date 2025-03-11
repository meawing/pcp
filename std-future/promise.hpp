#pragma once

#include <memory>

#include "state.hpp"

namespace future {

template <class T>
class Future;

template <class T>
class Promise {
public:
    Promise() = default;
    Promise(const Promise&) = delete;
    Promise(Promise&& other) = default;

    void SetValue(T value) && {
        // TODO: Your solution
    }

    void SetError(Error error) && {
        // TODO: Your solution
    }

private:
    friend class Future<T>;

    // Shared state...
    // TODO: Your solution
};

}  // namespace future
