#include <catch2/catch_all.hpp>

#include <lines/fibers/api.hpp>
#include <lines/time/api.hpp>

#include <vector>
#include <string>
#include <map>

#include "queue.hpp"

TEST_CASE("PingPong") {
    lines::SchedulerRun([]() {
        MPMCBlockingUnboundedQueue<std::string> first_queue;
        MPMCBlockingUnboundedQueue<std::string> second_queue;
        std::vector<std::string> answers;

        auto first = lines::Spawn([&]() {
            lines::SleepFor(100ms);
            first_queue.Push("ping");

            auto ans = second_queue.Pop();
            answers.push_back(std::move(ans));
        });

        auto second = lines::Spawn([&]() {
            auto ans = first_queue.Pop();
            answers.push_back(std::move(ans));

            lines::SleepFor(100ms);
            second_queue.Push("pong");
        });

        first.join();
        second.join();

        REQUIRE(answers[0] == "ping");
        REQUIRE(answers[1] == "pong");
    });
}

TEST_CASE("MultiProducer") {
    lines::SchedulerRun(
        []() {
            MPMCBlockingUnboundedQueue<std::pair<size_t, size_t>> stream;
            std::map<size_t, std::vector<size_t>> answers;

            const size_t num_threads = 8;
            const size_t stream_size = 50000;
            auto sink = lines::Spawn([&]() {
                for (size_t i = 0; i < num_threads * stream_size; ++i) {
                    auto [tid, num] = stream.Pop();
                    answers[tid].push_back(num);
                }
            });

            std::vector<lines::Handle> workers;
            for (size_t tid = 0; tid < num_threads; ++tid) {
                workers.emplace_back([tid, &stream]() {
                    for (size_t num = 0; num < stream_size; ++num) {
                        stream.Push(std::make_pair(tid, num));
                    }
                });
            }

            for (auto& worker : workers) {
                worker.join();
            }
            sink.join();

            for (const auto& [_, nums] : answers) {
                REQUIRE(nums.size() == stream_size);
                for (size_t i = 0; i < stream_size; ++i) {
                    REQUIRE(nums[i] == i);
                }
            }
        },
        /*num_runs=*/4);
}

TEST_CASE("MultiConsumer") {
    lines::SchedulerRun(
        []() {
            const size_t num_threads = 8;
            const size_t stream_size = 50000;

            MPMCBlockingUnboundedQueue<size_t> stream;
            std::vector<std::vector<size_t>> answers(num_threads);

            auto source = lines::Spawn([&stream]() {
                for (size_t i = 0; i < num_threads * stream_size; ++i) {
                    stream.Push(i);
                }
            });

            std::vector<lines::Handle> consumers;
            for (size_t tid = 0; tid < num_threads; ++tid) {
                consumers.emplace_back([&ans = answers[tid], &stream]() {
                    for (size_t num = 0; num < stream_size; ++num) {
                        ans.push_back(stream.Pop());
                    }
                });
            }

            for (auto& consumer : consumers) {
                consumer.join();
            }
            source.join();

            size_t sum = 0;
            for (const auto& ans : answers) {
                for (size_t num : ans) {
                    sum += num;
                }
            }
            REQUIRE(sum == (num_threads * stream_size) * (num_threads * stream_size - 1) / 2);
        },
        /*num_runs=*/4);
}
