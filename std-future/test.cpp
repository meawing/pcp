#include <catch2/catch_all.hpp>

#include <exception>

#include <lines/fibers/api.hpp>
#include <lines/time/api.hpp>
#include <lines/util/defer.hpp>
#include <lines/util/logger.hpp>

#include "future.hpp"
#include "unit.hpp"

TEST_CASE("SetValue") {
    lines::SchedulerRun([]() {
        auto [f, p] = future::GetTied<int>();

        auto handle = lines::Spawn([p = std::move(p)]() mutable {
            lines::SleepFor(100ms);

            std::move(p).SetValue(123);
        });
        lines::Defer defer([&handle]() mutable { handle.join(); });

        REQUIRE(std::move(f).Get() == 123);
    });
}

TEST_CASE("SetVoid") {
    lines::SchedulerRun([]() {
        auto [f, p] = future::GetTied<future::Unit>();

        auto handle = lines::Spawn([p = std::move(p)]() mutable {
            lines::SleepFor(100ms);

            std::move(p).SetValue(future::Unit{});
        });
        lines::Defer defer([&handle]() { handle.join(); });

        std::move(f).Get();
    });
}

namespace detail {

class CustomException {
public:
    CustomException(std::string what) : what_(std::move(what)) {
    }

    const std::string& What() const {
        return what_;
    }

private:
    std::string what_;
};

class ExceptionMatcher {
public:
    bool match(const CustomException& error) const {  // NOLINT
        return error.What() == expected_;
    }

    std::string toString() const {  // NOLINT
        return expected_;
    }

private:
    std::string expected_ = "promise error";
};

}  // namespace detail

TEST_CASE("SetError") {
    lines::SchedulerRun([]() {
        auto [f, p] = future::GetTied<int>();

        auto handle = lines::Spawn([p = std::move(p)]() mutable {
            lines::SleepFor(100ms);

            std::move(p).SetError(
                std::make_exception_ptr(detail::CustomException("promise error")));
        });
        lines::Defer defer([&handle]() { handle.join(); });

        REQUIRE_THROWS_MATCHES(std::move(f).Get(), detail::CustomException,
                               detail::ExceptionMatcher());
    });
}

namespace detail {

class WeirdObj {
public:
    WeirdObj(int value) : value_(value) {
    }

    WeirdObj(const WeirdObj&) = delete;
    WeirdObj(WeirdObj&&) = default;

    bool Compare(int value) {
        return value_ == value;
    }

private:
    int value_;
};

}  // namespace detail

TEST_CASE("SetWeird") {
    lines::SchedulerRun([]() {
        auto [f, p] = future::GetTied<detail::WeirdObj>();

        auto handle = lines::Spawn([p = std::move(p)]() mutable {
            lines::SleepFor(100ms);

            std::move(p).SetValue(detail::WeirdObj(123));
        });
        lines::Defer defer([&handle]() { handle.join(); });

        REQUIRE(std::move(f).Get().Compare(123));
    });
}

TEST_CASE("SetException") {
    lines::SchedulerRun([]() {
        auto [f, p] = future::GetTied<std::exception_ptr>();

        auto handle = lines::Spawn([p = std::move(p)]() mutable {
            lines::SleepFor(100ms);

            try {
                throw std::runtime_error("my error");
            } catch (...) {
                std::move(p).SetValue(std::current_exception());
            }
        });
        lines::Defer defer([&handle]() { handle.join(); });

        try {
            std::rethrow_exception(std::move(f).Get());
        } catch (const std::runtime_error& error) {
            REQUIRE(error.what() == std::string("my error"));
        }
    });
}

TEST_CASE("Lifetimes I") {
    lines::SchedulerRun([]() {
        auto [sync_f, sync_p] = future::GetTied<future::Unit>();
        auto [f, p] = future::GetTied<std::vector<int>>();

        auto handle = lines::Spawn([sync_p = std::move(sync_p), p = std::move(p)]() mutable {
            std::move(p).SetValue({1, 2, 3, 4, 5});

            std::move(sync_p).SetValue(future::Unit{});
        });
        lines::Defer defer([&handle]() { handle.join(); });

        std::move(sync_f).Get();

        auto numbers = std::move(f).Get();
        for (size_t i = 0; i < numbers.size(); ++i) {
            REQUIRE(numbers[i] == i + 1);
        }
    });
}

TEST_CASE("Lifetimes II") {
    lines::SchedulerRun([]() {
        auto [sync_f, sync_p] = future::GetTied<future::Unit>();
        bool set_with_dead_future = false;

        std::optional<lines::Handle> handle;
        {
            auto [f, p] = future::GetTied<std::vector<int>>();

            handle = lines::Spawn(
                [sync_f = std::move(sync_f), p = std::move(p), &set_with_dead_future]() mutable {
                    std::move(sync_f).Get();

                    std::move(p).SetValue({1, 2, 3, 4, 5});
                    set_with_dead_future = true;
                });
        }

        std::move(sync_p).SetValue(future::Unit{});

        handle->join();
        REQUIRE(set_with_dead_future);
    });
}
