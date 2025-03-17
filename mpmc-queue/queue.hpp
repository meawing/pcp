#pragma once

#include <lines/std/mutex.hpp>
#include <lines/std/condvar.hpp>

#include <queue>

template <class T>
class MPMCBlockingUnboundedQueue {
public:
    void Push(T elem) {
        std::lock_guard<lines::Mutex> guard(mutex_);
        queue_.push(std::move(elem));
        not_empty_.NotifyOne(); // Wake up one waiting consumer if any
    }

    T Pop() {
        std::lock_guard<lines::Mutex> guard(mutex_);
        
        // Wait while queue is empty
        while (queue_.empty()) {
            not_empty_.Wait(mutex_);
        }
        
        T result = std::move(queue_.front());
        queue_.pop();
        return result;
    }

private:
    lines::Mutex mutex_;              // Protects queue_
    lines::Condvar not_empty_;        // Signals when queue becomes non-empty
    std::queue<T> queue_;             // Underlying unbounded queue
};
