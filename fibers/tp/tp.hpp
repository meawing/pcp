#pragma once

#include <vector>

#include <lines/std/atomic.hpp>
#include <lines/std/thread.hpp>

#include "task.hpp"
#include "queue.hpp"
#include "wg.hpp"

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    void Submit(Task task);

    void Wait();

    static ThreadPool* This();

private:
    void Work();

private:
    // TODO: Your solution
};
