#include <catch2/catch_all.hpp>

#include <vector>

#include <lines/fibers/api.hpp>
#include <lines/util/compiler.hpp>

#include "spinlock.hpp"

TEST_CASE("MultiThread") {
    lines::SchedulerRun(
        []() {
            const int threads_count = 4;
            const int num_iterations = 1'000'000;

            int sum = 0;
            SpinLock lock;

            std::vector<lines::Handle> handles;
            handles.reserve(threads_count);
            for (int i = 0; i < threads_count; ++i) {
                handles.emplace_back(lines::Spawn([&]() {
                    for (int j = 0; j < num_iterations; ++j) {
                        lock.Lock();
                        ++sum;
                        lines::DoNotOptimize(sum);
                        lock.Unlock();
                    }
                }));
            }

            for (auto& h : handles) {
                h.join();
            }

            REQUIRE(num_iterations * threads_count == sum);
        },
        1);
}
