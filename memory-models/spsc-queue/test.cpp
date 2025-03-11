#include <catch2/catch_all.hpp>

#include <iostream>
#include <thread>
#include <numeric>

#include <lines/util/clock.hpp>

#include "queue.hpp"

TEST_CASE("SPSCQueue") {
    SPSCQueue<size_t> queue(5);
    const size_t limit = 10'000'000;
    const size_t num_runs = 5;
    std::vector<double> times;
    for (size_t i = 0; i < num_runs; ++i) {
        lines::WallClock clock;
        clock.Start();

        std::atomic<size_t> barrier = 0;
        std::thread producer([&]() {
            ++barrier;
            while (barrier != 2) {
            }

            clock.Start();

            for (size_t elem = 0; elem < limit; ++elem) {
                while (!queue.Push(elem)) {
                }
            }
        });

        std::thread consumer([&]() {
            ++barrier;
            while (barrier != 2) {
            }

            for (size_t current = 0; current < limit; ++current) {
                std::optional<size_t> actual;
                while (!(actual = queue.Pop())) {
                }

                REQUIRE(*actual == current);
            }

            auto time = clock.Finish();
            std::cout << time << " ms" << std::endl;
            times.push_back(time);
        });

        producer.join();
        consumer.join();
    }

    auto mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    std::cout << "mean: " << mean << " ms" << std::endl;
}
