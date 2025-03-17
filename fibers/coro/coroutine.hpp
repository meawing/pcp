#pragma once

#include <functional>

#include <lines/ctx/ctx.hpp>
#include <lines/ctx/trampoline.hpp>
#include <lines/ctx/stack.hpp>

using Routine = std::function<void()>;

class Coroutine : public lines::ITrampoline {
public:
    Coroutine(Routine routine);

    void Resume();
    static void Suspend();

    bool IsCompleted();

private:
    void Run() final;

private:
    // TODO: Your solution
};
