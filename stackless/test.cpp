#include <catch2/catch_all.hpp>

#include <lines/fibers/api.hpp>

#include <lines/util/move_only.hpp>
#include <lines/util/compiler.hpp>

#include <stackless/async_task.hpp>

#include <libassert/assert.hpp>

////////////////////////////////////////////////////////////////////////////////

struct MoveOnlyInt : public lines::MoveOnly {
    explicit MoveOnlyInt(int value) : value{value} {
    }

    MoveOnlyInt(MoveOnlyInt&& other) {
        value = other.value;
        other.value.reset();
    }

    MoveOnlyInt& operator=(MoveOnlyInt&& other) {
        value = other.value;
        other.value.reset();
        return *this;
    }

    std::optional<int> value;
};

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("BaseSemantics") {
    lines::SchedulerRun([] {
        coro::AsyncTask<MoveOnlyInt> task;
        REQUIRE_FALSE(task.IsValid());
        REQUIRE_THROWS_AS(std::move(task).Run(), coro::AsyncTaskInvalid);

        coro::AsyncTask<MoveOnlyInt> other(std::move(task));
        REQUIRE_FALSE(other.IsValid());
        REQUIRE_FALSE(task.IsValid());

        auto coro = []() -> coro::AsyncTask<MoveOnlyInt> {
            REQUIRE_THROWS_AS(co_await coro::AsyncTask<coro::Unit>(), coro::AsyncTaskInvalid);
            co_return MoveOnlyInt(42);
        };
        task = coro();
        REQUIRE(task.IsValid());

        other = std::move(task);
        REQUIRE(other.IsValid());
        REQUIRE_FALSE(task.IsValid());

        auto result = std::move(other).Run().get();
        REQUIRE(result.value.has_value());
        REQUIRE(*result.value == 42);
    });
}

TEST_CASE("Laziness") {
    lines::SchedulerRun([] {
        bool started = false;
        auto coro = [&]() -> coro::AsyncTask<int> {
            started = true;
            co_return 2;
        };
        auto task = coro();
        REQUIRE(!started);
        REQUIRE(std::move(task).Run().get() == 2);
        REQUIRE(started);
    });
}

TEST_CASE("NoMemoryLeakOfNonStartedTask") {
    lines::SchedulerRun([] {
        auto shared_int = std::make_shared<int>(42);
        auto coro = [](std::shared_ptr<int>) -> coro::AsyncTask<int> {
            co_return 1;
        };
        coro(shared_int);
    });
}

TEST_CASE("NestedCoros") {
    lines::SchedulerRun([] {
        bool inner_coro_started = false;
        auto inner_coro = [&]() -> coro::AsyncTask<MoveOnlyInt> {
            inner_coro_started = true;
            co_return MoveOnlyInt(1);
        };

        auto coro = [&]() -> coro::AsyncTask<int> {
            auto task = inner_coro();
            REQUIRE(!inner_coro_started);
            auto result = co_await std::move(task);
            REQUIRE_FALSE(task.IsValid());
            REQUIRE(inner_coro_started);
            co_return *result.value + 1;
        };

        REQUIRE(coro().Run().get() == 2);
    });
}

TEST_CASE("StackOverflow") {
    lines::SchedulerRun([] {
        // If you don't know what to do with this test, you may find this article
        // about symmertic transfer usefull:
        // https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
        constexpr int64_t kIterCount = 1'000'000;

        auto inner_coro = [](int64_t index) -> coro::AsyncTask<int64_t> {
            std::array<int, 10000> arr;
            lines::DoNotOptimize(arr);
            co_return index;
        };

        auto coro = [&]() -> coro::AsyncTask<int64_t> {
            int64_t result = 0;
            for (int i = 0; i < kIterCount; ++i) {
                result += co_await inner_coro(i);
            }
            co_return result;
        };

        REQUIRE(coro().Run().get() == (kIterCount * (kIterCount - 1)) / 2);
    });
}

TEST_CASE("Exceptions") {
    lines::SchedulerRun([] {
        auto inner_coro = [](bool should_throw) -> coro::AsyncTask<int64_t> {
            if (should_throw) {
                throw std::runtime_error("Oops");
            }
            co_return 42;
        };

        auto coro = [&]() -> coro::AsyncTask<int64_t> {
            REQUIRE(co_await inner_coro(false) == 42);
            REQUIRE_THROWS_WITH(co_await inner_coro(true), "Oops");
            co_await inner_coro(true);
            ASSERT(false);
            std::unreachable();
        };

        REQUIRE_THROWS_WITH(coro().Run().get(), "Oops");
    });
}

TEST_CASE("CoroIsDestroyedAfterFinishingExecution") {
    lines::SchedulerRun([] {
        auto inner_coro = [](std::shared_ptr<int> value) -> coro::AsyncTask<int> {
            co_return *value;
        };

        auto coro = [&](std::shared_ptr<int> value) -> coro::AsyncTask<int> {
            REQUIRE(value.use_count() == 2);
            auto task = inner_coro(value);
            REQUIRE(value.use_count() == 3);
            co_await std::move(task);
            REQUIRE(value.use_count() == 2);
            co_return *value;
        };

        auto value = std::make_shared<int>(42);
        auto task = coro(value);

        REQUIRE(value.use_count() == 2);
        REQUIRE(std::move(task).Run().get() == 42);
        REQUIRE(value.use_count() == 1);
    });
}
////////////////////////////////////////////////////////////////////////////////
