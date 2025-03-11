#include <catch2/catch_all.hpp>

#include <algorithm>
#include <thread>
#include <set>
#include <random>
#include <cstdint>
#include <chrono>
#include <mutex>

#include "flat-combiner.h"

////////////////////////////////////////////////////////////////////////////////

namespace flat_combining_heap {

enum class RequestType {
    kPush = 0,
    kPop,
};

struct Push {
    int64_t value;
};

struct Pop {
    int64_t result;
};

struct HeapMessage {
    RequestType type;
    union {
        Push push;
        Pop pop;
    } data = {};
};

class Heap : public FlatCombinerBase<HeapMessage, Heap> {
public:
    Heap(int concurrency) : FlatCombinerBase<HeapMessage, Heap>(concurrency) {
    }

    void Push(void* cookie, int64_t value) {
        auto* message = static_cast<Message*>(cookie);
        message->body.type = RequestType::kPush;
        message->body.data = {.push = {value}};
        Run(message);
    }

    int64_t Pop(void* cookie) {
        auto* message = static_cast<Message*>(cookie);
        message->body.type = RequestType::kPop;
        Run(message);
        return message->body.data.pop.result;
    }

    void Dispatch(Message* message) {
        switch (message->body.type) {
            case RequestType::kPush: {
                tree_.insert(message->body.data.push.value);
                break;
            }
            case RequestType::kPop: {
                auto it = tree_.begin();
                message->body.data.pop.result = *it;
                tree_.erase(it);
                break;
            }
            default:
                std::abort();
        }
    }

private:
    std::multiset<int64_t> tree_;
};

}  // namespace flat_combining_heap

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Simple") {
    int length = 10;

    auto data = std::vector<int64_t>(length);
    for (int i = 0; i < length; ++i) {
        data[i] = i;
    }

    auto gen = std::mt19937(42);
    std::shuffle(data.begin(), data.end(), gen);

    auto heap = flat_combining_heap::Heap(1);
    void* cookie = heap.CheckIn();

    for (auto it : data) {
        heap.Push(cookie, it);
    }

    for (int i = 0; i < length; ++i) {
        REQUIRE(i == heap.Pop(cookie));
    }
}

////////////////////////////////////////////////////////////////////////////////

static auto checked_in = std::atomic<int>(0);

static auto nthread = 8;

////////////////////////////////////////////////////////////////////////////////

void TestConcurrentSingleThreadTask(flat_combining_heap::Heap* heap, int operations) {
    void* cookie = heap->CheckIn();

    checked_in.fetch_add(1);
    while (checked_in.load() != nthread) {
        // Spin.
    }

    for (int i = 0; i < operations; ++i) {
        heap->Push(cookie, i);
    }
}

TEST_CASE("Concurrent") {
    int operations = 10000;

    for (int iteration = 0; iteration < 10; ++iteration) {
        checked_in.store(0);

        auto heap = flat_combining_heap::Heap(nthread + 1);  // +1 for this thread.
        void* cookie = heap.CheckIn();

        std::vector<std::thread> tp;
        for (int i = 0; i < nthread; ++i) {
            tp.emplace_back(TestConcurrentSingleThreadTask, &heap, operations);
        }
        for (int i = 0; i < nthread; ++i) {
            tp[i].join();
        }

        for (int i = 0; i < operations; ++i) {
            for (int j = 0; j < nthread; ++j) {
                REQUIRE(heap.Pop(cookie) == i);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

namespace flat_combining_worker {

enum class RequestType {
    kWork = 0,
};

struct Work {};

struct WorkerMessage {
    RequestType type;
    union {
        Work work;
    } data = {};
};

class Worker : public FlatCombinerBase<WorkerMessage, Worker> {
public:
    Worker(int concurrency) : FlatCombinerBase<WorkerMessage, Worker>(concurrency) {
    }

    void Work(void* cookie) {
        auto* message = static_cast<Message*>(cookie);
        message->body.type = RequestType::kWork;
        message->body.data = {};
        Run(message);
    }

    void Dispatch(Message* message) {
        switch (message->body.type) {
            case RequestType::kWork: {
                volatile char counter = 0;
                for (int i = 0; i < 256; ++i) {
                    counter = counter + 1;
                }
                break;
            }
            default:
                std::abort();
        }
    }
};

}  // namespace flat_combining_worker

namespace mutexed_worker {

class Worker {
public:
    void Work() {
        auto guard = std::lock_guard(mx_);
        volatile char counter = 0;
        for (int i = 0; i < 256; ++i) {
            counter = counter + 1;
        }
    }

private:
    std::mutex mx_;
};

}  // namespace mutexed_worker

void BenchFlatCombiningImpl(flat_combining_worker::Worker* worker,
                            std::atomic<int64_t>* total_time) {
    void* cookie = worker->CheckIn();

    checked_in.fetch_add(1);
    while (checked_in.load() != nthread) {
        // Spin.
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        worker->Work(cookie);
    }

    auto stop_time = std::chrono::high_resolution_clock::now();

    total_time->fetch_add(
        std::chrono::duration_cast<std::chrono::nanoseconds>(stop_time - start_time).count());
}

void BenchMutexedImpl(mutexed_worker::Worker* worker, std::atomic<int64_t>* total_time) {
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        worker->Work();
    }

    auto stop_time = std::chrono::high_resolution_clock::now();

    total_time->fetch_add(
        std::chrono::duration_cast<std::chrono::nanoseconds>(stop_time - start_time).count());
}

TEST_CASE("Bench") {
    std::atomic<int64_t> flat_combining = 0;
    std::atomic<int64_t> mutexed = 0;

    for (int iteration = 0; iteration < 200; ++iteration) {
        {
            auto worker = mutexed_worker::Worker();
            std::vector<std::thread> tp;
            for (int i = 0; i < nthread; ++i) {
                tp.emplace_back(BenchMutexedImpl, &worker, &mutexed);
            }
            for (int i = 0; i < nthread; ++i) {
                tp[i].join();
            }
        }
        {
            checked_in.store(0);

            auto worker = flat_combining_worker::Worker(nthread);
            std::vector<std::thread> tp;
            for (int i = 0; i < nthread; ++i) {
                tp.emplace_back(BenchFlatCombiningImpl, &worker, &flat_combining);
            }
            for (int i = 0; i < nthread; ++i) {
                tp[i].join();
            }
        }
    }

    REQUIRE((1.0 * mutexed / flat_combining) > 1.8);
}

////////////////////////////////////////////////////////////////////////////////
