#include <catch2/catch_all.hpp>

#include <thread>
#include <future>

#include "channel.h"

#include <lines/util/compiler.hpp>

struct MoveOnlyStruct {
    MoveOnlyStruct(int value) : value_(value) {
    }

    MoveOnlyStruct(const MoveOnlyStruct&) = delete;
    MoveOnlyStruct& operator=(const MoveOnlyStruct&) = default;
    MoveOnlyStruct(MoveOnlyStruct&&) = default;
    MoveOnlyStruct& operator=(MoveOnlyStruct&&) = default;

private:
    int value_;
};

TEST_CASE("ChannelOneMessage") {
    UnbufferedChannel<std::string> chan;
    const std::string message = "Hello, world!";
    std::string received_message;

    std::thread receiver([&]() { received_message = chan.Pop().value(); });

    chan.Push(message);
    receiver.join();
    REQUIRE(received_message == message);
}

TEST_CASE("ChannelMoveOnly") {
    UnbufferedChannel<MoveOnlyStruct> chan;

    std::thread receiver([&]() { auto _ = std::move(chan.Pop().value()); });

    chan.Push(MoveOnlyStruct(3));
    receiver.join();
}

TEST_CASE("ChannelClosingException") {
    UnbufferedChannel<std::string> chan;
    const std::string message = "Too late...";

    chan.Close();
    REQUIRE_THROWS(chan.Push(message));
}

TEST_CASE("ChannelDeadLockPushFirst") {
    using namespace std::chrono_literals;

    UnbufferedChannel<std::string> chan;
    std::future<void> future = std::async(std::launch::async, [&chan]() {
        const std::string message = "Hello, world!";
        chan.Push(message);
        auto _ = chan.Pop();
    });

    auto future_status = future.wait_for(1s);
    REQUIRE(future_status == std::future_status::timeout);
    chan.Close();
    try {
        future.get();
    } catch (...) {
    }
}

TEST_CASE("ChannelDeadLockPopFirst") {
    using namespace std::chrono_literals;

    UnbufferedChannel<std::string> chan;
    std::optional<std::string> maybe = "Some string";
    std::future<void> future = std::async(std::launch::async, [&chan, &maybe]() {
        const std::string message = "Hello, world!";
        maybe = chan.Pop();
        chan.Push(message);
    });

    auto future_status = future.wait_for(1s);
    REQUIRE(future_status == std::future_status::timeout);
    chan.Close();
    try {
        future.get();
    } catch (...) {
        REQUIRE(!maybe.has_value());
    }
}

TEST_CASE("ChannelCycle") {
    UnbufferedChannel<int> chan;
    const int iters = 10;
    const int offset = 100;

    int receiver_sum = 0;
    std::thread receiver([&]() {
        for (int i = 0; i < iters; ++i) {
            int x = chan.Pop().value();
            receiver_sum += x * i;
        }
    });

    for (int i = 0; i < iters; ++i) {
        chan.Push(offset + i);
    }
    receiver.join();

    int main_sum = [&]() {
        int sum = 0;
        for (int i = 0; i < iters; ++i) {
            sum += (offset + i) * i;
        }
        return sum;
    }();
    REQUIRE(receiver_sum == main_sum);
}

TEST_CASE("ChannelMPMC") {
    int num_threads = 4;
    int iters = 100;

    UnbufferedChannel<int> chan;
    std::vector<std::thread> readers(num_threads);
    std::vector<std::thread> writers(num_threads);

    std::atomic<int> reader_sum = 0;
    for (std::thread& reader : readers) {
        reader = std::thread([&]() {
            for (int i = 0; i < iters; ++i) {
                reader_sum += chan.Pop().value();
            }
        });
    }

    for (std::thread& writer : writers) {
        writer = std::thread([&]() {
            for (int i = 0; i < iters; ++i) {
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
        for (int i = 0; i < iters; ++i) {
            sum += i * num_threads;
        }
        return sum;
    }();
    REQUIRE(reader_sum == main_sum);
}
