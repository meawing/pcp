#pragma once

#include <libassert/assert.hpp>

#include "coro/coroutine.hpp"
#include "tp/tp.hpp"

class Fiber {
public:
    // TODO: Your solution

    void Resume();
    void Suspend();

    static Fiber* This();

private:
    void Schedule();

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

void Yield();

}  // namespace api
