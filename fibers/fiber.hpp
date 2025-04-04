#pragma once

#include <libassert/assert.hpp>
#include <lines/util/thread_local.hpp>

#include "coro/coroutine.hpp"
#include "tp/tp.hpp"


class Fiber {
public:
    // TODO: Your solution

    void Resume() {
        // TODO: Your solution
    }
    void Suspend() {
        // TODO: Your solution
    }

    static Fiber* This() {
        // TODO: Your solution
    }

private:
    void Schedule() {
        // TODO: Your solution
    }

private:
    // TODO: Your solution
};

namespace api {

template <class F>
void Spawn(F&& f, ThreadPool* tp) {
    // TODO: Your solution
}

template <class F>
void Spawn(F&& f) {
    // TODO: Your solution
}

void Yield() {
    // TODO: Your solution
}

}  // namespace api
