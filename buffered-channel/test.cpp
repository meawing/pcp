#include <catch2/catch_all.hpp>

#include <thread>
#include <chrono>
#include <future>

#include "channel.h"

#include <lines/util/compiler.hpp>

struct MoveOnlyStruct {
    MoveOnlyStruct(int value) : value_(value) {
    }

    MoveOnlyStruct(const MoveOnlyStruct&) = delete;
    MoveOnlyStruct(MoveOnlyStruct&&) = default;

private:
    int value_;
};

TEST_CASE("BufferedChannelOneMessage") {
    BufferedChannel<std::string> chan(1);
    const std::string message = "Hello, world!";
    std::string received_message;

    std::thread receiver([&]() { received_message = chan.Pop().value(); });

    chan.Push(message);
    receiver.join();
    REQUIRE(received_message == message);
}

TEST_CASE("BufferedChannelMoveOnly") {
    BufferedChannel<MoveOnlyStruct> chan(1);

    std::thread receiver([&]() { auto _ = std::move(chan.Pop().value()); });

    chan.Push(MoveOnlyStruct(3));
    receiver.join();
}

TEST_CASE("BufferedChannelDeadLock") {
    using namespace std::chrono_literals;
    const int channel_capacity = 3;

    {
        BufferedChannel<std::string> chan(channel_capacity);
        std::future<void> future = std::async(std::launch::async, [&chan]() {
            const std::string message = "Hello, world!";
            for (int i = 0; i < channel_capacity + 1; ++i) {
                chan.Push(message);
            }
            for (int i = 0; i < channel_capacity + 1; ++i) {
                auto _ = chan.Pop();
            }
        });

        auto future_status = future.wait_for(1s);
        REQUIRE(future_status == std::future_status::timeout);
        chan.Close();
        try {
            future.get();
        } catch (...) {
        }
    }
    {
        BufferedChannel<std::string> chan(channel_capacity);
        std::future<void> future = std::async(std::launch::async, [&chan]() {
            const std::string message = "Hello, world!";
            auto _ = chan.Pop();
            for (int i = 0; i < channel_capacity + 1; ++i) {
                chan.Push(message);
            }
        });

        auto future_status = future.wait_for(1s);
        REQUIRE(future_status == std::future_status::timeout);
        chan.Close();
        try {
            future.get();
        } catch (...) {
        }
    }
}

bool TestMPMC(const int capacity, const int num_producers, const int num_consumers,
              const int producer_duration, const int consumer_duration) {
    BufferedChannel<int> chan(capacity);
    std::vector<std::thread> readers(num_consumers);
    std::vector<std::thread> writers(num_producers);

    std::atomic<int> sum = 0;
    for (std::thread& reader : readers) {
        reader = std::thread([&]() {
            for (int i = 0; i < num_producers; ++i) {
                if (consumer_duration != 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(consumer_duration));
                }
                sum += chan.Pop().value();
            }
        });
    }

    for (std::thread& writer : writers) {
        writer = std::thread([&]() {
            for (int i = 0; i < num_consumers; ++i) {
                if (producer_duration != 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(producer_duration));
                }
                chan.Push(i);
            }
        });
    }

    for (std::thread& reader : readers) {
        reader.join();
    }
    for (std::thread& writer : writers) {
        writer.join();
    }

    int main_sum = [&]() {
        int sum = 0;
        for (int i = 0; i < num_consumers; ++i) {
            sum += i * num_producers;
        }
        return sum;
    }();

    return sum == main_sum;
}

TEST_CASE("BufferedChannelMPMC") {
    const int channel_capacity = 10;

    REQUIRE(TestMPMC(channel_capacity, 4, 4, 0, 0));
    REQUIRE(TestMPMC(channel_capacity, 4, 4, 100, 0));
    REQUIRE(TestMPMC(channel_capacity, 4, 4, 0, 100));
    REQUIRE(TestMPMC(channel_capacity, 4, 4, 100, 100));
    REQUIRE(TestMPMC(channel_capacity, 15, 9, 50, 70));
    REQUIRE(TestMPMC(channel_capacity, 6, 21, 40, 10));
}
