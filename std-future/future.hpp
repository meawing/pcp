#pragma once

#include <utility>

#include "promise.hpp"
#include "state.hpp"

namespace future {

template <class T>
class Future {
public:
    Future(Promise<T>& promise) {
        // TODO: Your solution
    }

    Future(const Future&) = delete;
    Future(Future&&) = default;

    T Get() && {
        // TODO: Your solution
    }

private:
    // Shared state...
    // TODO: Your solution
};

template <class T>
std::pair<Future<T>, Promise<T>> GetTied() {
    Promise<T> p;
    Future<T> f(p);

    return std::make_pair(std::move(f), std::move(p));
}

}  // namespace future
