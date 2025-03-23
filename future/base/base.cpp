#include <catch2/catch_all.hpp>
#include <lines/util/compiler.hpp>
#include <lines/util/move_only.hpp>
#include <lines/fibers/api.hpp>

#include <future/future.hpp>

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

struct CopyableInt {
    explicit CopyableInt(int value) : value{value} {
    }

    CopyableInt(const CopyableInt& other) {
        value = other.value;
    }

    CopyableInt& operator=(const CopyableInt& other) {
        value = other.value;
        return *this;
    }

    CopyableInt(CopyableInt&& other) {
        value = other.value;
        other.value.reset();
    }

    CopyableInt& operator=(CopyableInt&& other) {
        value = other.value;
        other.value.reset();
        return *this;
    }

    std::optional<int> value;
};

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("BadFutureUsage") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<int>();

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        REQUIRE_THROWS_AS(f.GetResult(), future::FutureNotReady);
        REQUIRE_THROWS_AS(std::move(f).GetResult(), future::FutureNotReady);
        REQUIRE_THROWS_AS(static_cast<const future::Future<int>&>(f).GetResult(),
                          future::FutureNotReady);

        REQUIRE_THROWS_AS(f.GetValue(), future::FutureNotReady);
        REQUIRE_THROWS_AS(std::move(f).GetValue(), future::FutureNotReady);
        REQUIRE_THROWS_AS(static_cast<const future::Future<int>&>(f).GetValue(),
                          future::FutureNotReady);

        REQUIRE_THROWS_AS(f.HasException(), future::FutureNotReady);

        // Move out future.
        {
            auto fmov = std::move(f);
            REQUIRE(fmov.IsValid());
            REQUIRE(!fmov.IsReady());
        }

        REQUIRE(!f.IsValid());
        REQUIRE_THROWS_AS(f.IsReady(), future::FutureInvalid);

        REQUIRE_THROWS_AS(f.GetResult(), future::FutureInvalid);
        REQUIRE_THROWS_AS(std::move(f).GetResult(), future::FutureInvalid);
        REQUIRE_THROWS_AS(static_cast<const future::Future<int>&>(f).GetResult(),
                          future::FutureInvalid);

        REQUIRE_THROWS_AS(f.GetValue(), future::FutureInvalid);
        REQUIRE_THROWS_AS(std::move(f).GetValue(), future::FutureInvalid);
        REQUIRE_THROWS_AS(static_cast<const future::Future<int>&>(f).GetValue(),
                          future::FutureInvalid);

        REQUIRE_THROWS_AS(f.HasException(), future::FutureInvalid);

        REQUIRE(p.IsValid());
    });
}

TEST_CASE("BadPromiseUsage") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<int>();

        REQUIRE(p.IsValid());
        REQUIRE(!p.IsFulfilled());

        std::move(p).SetValue(42);

        REQUIRE(p.IsValid());
        REQUIRE(p.IsFulfilled());

        REQUIRE_THROWS_AS(std::move(p).SetValue(0), future::PromiseAlreadySatisfied);
        REQUIRE_THROWS_AS(
            std::move(p).SetError(std::make_exception_ptr(std::runtime_error("Failed"))),
            future::PromiseAlreadySatisfied);

        // Move out promise.
        {
            auto tmp = std::move(p);

            REQUIRE(tmp.IsValid());
            REQUIRE(tmp.IsFulfilled());
        }

        REQUIRE(!p.IsValid());

        REQUIRE_THROWS_AS(std::move(p).SetValue(0), future::PromiseInvalid);
        REQUIRE_THROWS_AS(
            std::move(p).SetError(std::make_exception_ptr(std::runtime_error("Failed"))),
            future::PromiseInvalid);

        REQUIRE(f.IsValid());
        REQUIRE(f.IsReady());
    });
}

TEST_CASE("SetValue") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();
        REQUIRE(p.IsValid());
        REQUIRE(f.IsValid());

        REQUIRE(!f.IsReady());
        REQUIRE(!p.IsFulfilled());

        std::move(p).SetValue(42);

        REQUIRE(f.IsReady());
        REQUIRE(p.IsFulfilled());
        REQUIRE(!f.HasException());

        // Should complete immediately.
        REQUIRE(&f.Wait() == &f);

        REQUIRE(f.GetValue().value == 42);
        REQUIRE(std::move(f).GetValue().value == 42);
        REQUIRE(static_cast<const future::Future<MoveOnlyInt>&>(f).GetValue().value == 42);

        MoveOnlyInt value = std::move(f).GetValue();
        REQUIRE(value.value == 42);

        REQUIRE(f.GetValue().value == std::nullopt);
    });
}

TEST_CASE("GetValueCopyable") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<CopyableInt>();

        {
            CopyableInt value(42);
            std::move(p).SetValue(value);
            REQUIRE(value.value.has_value());
        }

        {
            auto value = f.GetValue();
            REQUIRE(value.value == 42);
            REQUIRE(f.GetValue().value == 42);
        }

        {
            auto value = std::move(f).GetValue();
            REQUIRE(value.value == 42);
            REQUIRE(!f.GetValue().value.has_value());
        }
    });
}

TEST_CASE("GetResultCopyable") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<CopyableInt>();

        std::move(p).SetValue(CopyableInt(42));

        {
            auto res = f.GetResult();
            REQUIRE(res.has_value());
            REQUIRE(res->value == 42);
            REQUIRE(f.GetValue().value == 42);
            REQUIRE(f.GetResult()->value == 42);
        }

        {
            auto res = std::move(f).GetResult();
            REQUIRE(res.has_value());
            REQUIRE(res->value == 42);
            REQUIRE(!f.GetResult()->value.has_value());
        }
    });
}

TEST_CASE("SetError") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error{"Failed"}));

        REQUIRE(f.IsReady());
        REQUIRE(p.IsFulfilled());
        REQUIRE(f.HasException());

        REQUIRE_THROWS_WITH(f.GetValue(), "Failed");
        REQUIRE_THROWS_WITH(std::move(f).GetValue(), "Failed");
        REQUIRE_THROWS_WITH(static_cast<const future::Future<MoveOnlyInt>&>(f).GetValue(),
                            "Failed");
    });
}

TEST_CASE("PromiseIsDestroyed") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();

        {
            auto tmp = std::move(p);
        }

        REQUIRE(f.IsReady());
        REQUIRE(f.HasException());

        REQUIRE_THROWS_AS(f.GetValue(), future::PromiseBroken);
    });
}

TEMPLATE_TEST_CASE_SIG("SetMultithreaded", "", ((bool kBusyWait), kBusyWait), true, false) {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();

        // Check that happens-before relation holds.
        int shared = 0;

        lines::DoNotOptimize(shared);

        auto consumer = lines::Spawn([f = std::move(f), &shared]() mutable {
            if constexpr (kBusyWait) {
                f = std::move(f).Wait();
            } else {
                while (!f.IsReady()) {
                }
            }
            MoveOnlyInt result = std::move(f).GetValue();
            REQUIRE(result.value == 42);
            REQUIRE(shared == 42);
        });

        auto producer = lines::Spawn([p = std::move(p), &shared]() mutable {
            shared = 42;
            std::move(p).SetValue(42);
        });

        consumer.join();
        producer.join();
    });
}

TEST_CASE("Subscribe") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();

        bool is_called = false;
        std::move(f).Subscribe([&](future::Result<MoveOnlyInt>&& result) noexcept {
            is_called = true;
            REQUIRE(result->value == 42);
        });

        std::move(p).SetValue(42);

        REQUIRE(is_called);
        REQUIRE(!f.IsValid());
    });
}

TEST_CASE("SubscribeBadUsage") {
    lines::SchedulerRun([] {
        future::Future<MoveOnlyInt> f;
        REQUIRE_THROWS_AS(std::move(f).Subscribe([](auto&&) noexcept {}), future::FutureInvalid);
    });
}

TEST_CASE("SubscribeAndError") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));

        bool is_called = false;
        std::move(f).Subscribe([&](future::Result<MoveOnlyInt>&& result) noexcept {
            is_called = true;
            REQUIRE_THROWS_WITH(std::rethrow_exception(result.error()), "oops");
        });

        REQUIRE(is_called);
        REQUIRE(!f.IsValid());
    });
}
