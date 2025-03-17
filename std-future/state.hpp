#pragma once

#include <variant>
#include <exception>
#include <utility>
#include <optional>

#include <lines/std/atomic.hpp>
#include <lines/std/condvar.hpp>
#include <lines/std/mutex.hpp>

namespace future {

using Error = std::exception_ptr;

namespace detail {

template <class T>
class SharedState {
private:
    struct Container {
        // TODO: Your solution
    };

public:
    SharedState() = default;

    void SetValue(T value) {
        // TODO: Your solution
    }

    void SetError(Error error) {
        // TODO: Your solution
    }

    T Get() {
        // TODO: Your solution
    }

private:
    // Some container for value or error
    // TODO: Your solution
};

}  // namespace detail

}  // namespace future
