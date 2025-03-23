#include <catch2/catch_all.hpp>
#include <lines/util/move_only.hpp>
#include <lines/fibers/api.hpp>

#include <future/future.hpp>
#include <future/combine.hpp>

#include <vector>

////////////////////////////////////////////////////////////////////////////////

struct MoveOnlyInt : public lines::MoveOnly {
    explicit MoveOnlyInt(int value) : value{value} {
    }

    int value = 1;
};

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("CollectAll") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<std::string>();

        auto f = future::CollectAll(std::move(f1), std::move(f2));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p1).SetValue(1);
        REQUIRE(!f.IsReady());

        std::move(p2).SetValue("42");
        REQUIRE(f.IsReady());

        auto res = std::move(f).GetValue();
        REQUIRE(std::get<0>(res)->value == 1);
        REQUIRE(std::get<1>(res).value() == "42");
    });
}

TEST_CASE("CollectAllSameTypes") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<std::string>();
        auto [f2, p2] = future::GetTied<std::string>();
        auto [f3, p3] = future::GetTied<std::string>();

        auto f = future::CollectAll(std::move(f1), std::move(f2), std::move(f3));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());
        REQUIRE(!f3.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p1).SetValue("41");
        REQUIRE(!f.IsReady());

        std::move(p2).SetValue("42");
        REQUIRE(!f.IsReady());

        std::move(p3).SetValue("43");
        REQUIRE(f.IsReady());

        auto res = std::move(f).GetValue();
        REQUIRE(std::get<0>(res).value() == "41");
        REQUIRE(std::get<1>(res).value() == "42");
        REQUIRE(std::get<2>(res).value() == "43");
    });
}

TEST_CASE("CollectAllWithError") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<std::string>();
        auto [f3, p3] = future::GetTied<std::vector<int>>();

        auto f = future::CollectAll(std::move(f1), std::move(f2), std::move(f3));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());
        REQUIRE(!f3.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p1).SetValue(1);
        REQUIRE(!f.IsReady());

        std::move(p3).SetValue(std::vector{1, 2, 3});
        REQUIRE(!f.IsReady());

        std::move(p2).SetError(std::make_exception_ptr(std::runtime_error("second")));
        REQUIRE(f.IsReady());

        REQUIRE(std::get<0>(f.GetValue())->value == 1);
        REQUIRE_THROWS_WITH(std::rethrow_exception(std::get<1>(f.GetValue()).error()), "second");
        REQUIRE(std::get<2>(f.GetValue()).value() == std::vector{1, 2, 3});
    });
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Collect") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<std::string>();

        auto f = future::Collect(std::move(f1), std::move(f2));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p2).SetValue("42");
        REQUIRE(!f.IsReady());

        std::move(p1).SetValue(1);
        REQUIRE(f.IsReady());

        REQUIRE(std::get<0>(f.GetValue()).value == 1);
        REQUIRE(std::get<1>(f.GetValue()) == "42");
    });
}

TEST_CASE("CollectSameTypes") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<int>();
        auto [f2, p2] = future::GetTied<int>();
        auto [f3, p3] = future::GetTied<int>();

        auto f = future::Collect(std::move(f1), std::move(f2), std::move(f3));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());
        REQUIRE(!f3.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p2).SetValue(42);
        REQUIRE(!f.IsReady());

        std::move(p1).SetValue(41);
        REQUIRE(!f.IsReady());

        std::move(p3).SetValue(43);
        REQUIRE(f.IsReady());

        REQUIRE(std::get<0>(f.GetValue()) == 41);
        REQUIRE(std::get<1>(f.GetValue()) == 42);
        REQUIRE(std::get<2>(f.GetValue()) == 43);
    });
}

TEST_CASE("CollectWithError") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<std::string>();
        auto [f3, p3] = future::GetTied<std::vector<int>>();

        auto f = future::Collect(std::move(f1), std::move(f2), std::move(f3));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());
        REQUIRE(!f3.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p1).SetValue(1);
        REQUIRE(!f.IsReady());

        std::move(p2).SetError(std::make_exception_ptr(std::runtime_error("oops")));
        REQUIRE(f.IsReady());

        std::move(p3).SetValue(std::vector{1, 2, 3});
        REQUIRE(f.IsReady());

        REQUIRE_THROWS_WITH(f.GetValue(), "oops");
    });
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("CollectAny") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<std::string>();

        auto f = future::CollectAny(std::move(f1), std::move(f2));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p2).SetValue("42");
        REQUIRE(f.IsReady());

        REQUIRE(std::get<1>(f.GetValue()).value() == "42");

        std::move(p1).SetValue(1);
        REQUIRE(f.IsReady());

        REQUIRE(std::get<1>(f.GetValue()).value() == "42");
    });
}

TEST_CASE("CollectAnySameTypes") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<MoveOnlyInt>();
        auto [f3, p3] = future::GetTied<MoveOnlyInt>();

        auto f = future::CollectAny(std::move(f1), std::move(f2), std::move(f3));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());
        REQUIRE(!f3.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p3).SetValue(3);
        REQUIRE(f.IsReady());

        REQUIRE(std::get<2>(f.GetValue()).value().value == 3);

        std::move(p1).SetValue(1);
        REQUIRE(f.IsReady());

        REQUIRE(std::get<2>(f.GetValue()).value().value == 3);
    });
}

TEST_CASE("CollectAnyWithError") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<std::string>();
        auto [f3, p3] = future::GetTied<std::vector<int>>();

        auto f = future::CollectAny(std::move(f1), std::move(f2), std::move(f3));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());
        REQUIRE(!f3.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p2).SetError(std::make_exception_ptr(std::runtime_error("second")));
        REQUIRE(f.IsReady());

        REQUIRE_THROWS_WITH(std::rethrow_exception(std::get<1>(f.GetValue()).error()), "second");

        std::move(p1).SetValue(1);
        REQUIRE(f.IsReady());

        std::move(p3).SetError(std::make_exception_ptr(std::runtime_error("third")));
        REQUIRE(f.IsReady());

        REQUIRE_THROWS_WITH(std::rethrow_exception(std::get<1>(f.GetValue()).error()), "second");
    });
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("CollectAnyWithoutException") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<std::string>();

        auto f = future::CollectAnyWithoutException(std::move(f1), std::move(f2));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p2).SetValue("42");
        REQUIRE(f.IsReady());

        REQUIRE(std::get<1>(f.GetValue()) == "42");

        std::move(p1).SetValue(1);
        REQUIRE(f.IsReady());

        REQUIRE(std::get<1>(f.GetValue()) == "42");
    });
}

TEST_CASE("CollectAnyWithoutExceptionWithErrors") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();
        auto [f2, p2] = future::GetTied<std::string>();
        auto [f3, p3] = future::GetTied<std::vector<int>>();

        auto f = future::CollectAnyWithoutException(std::move(f1), std::move(f2), std::move(f3));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());
        REQUIRE(!f3.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p2).SetError(std::make_exception_ptr(std::runtime_error("second")));
        REQUIRE(!f.IsReady());

        std::move(p1).SetValue(1);
        REQUIRE(f.IsReady());

        REQUIRE(std::get<0>(f.GetValue()).value == 1);

        std::move(p3).SetError(std::make_exception_ptr(std::runtime_error("third")));
        REQUIRE(f.IsReady());

        REQUIRE(std::get<0>(f.GetValue()).value == 1);
    });
}

TEST_CASE("CollectAnyWithoutExceptionWithAllErrors") {
    lines::SchedulerRun([] {
        auto [f1, p1] = future::GetTied<std::string>();
        auto [f2, p2] = future::GetTied<std::string>();
        auto [f3, p3] = future::GetTied<std::string>();

        auto f = future::CollectAnyWithoutException(std::move(f1), std::move(f2), std::move(f3));

        REQUIRE(!f1.IsValid());
        REQUIRE(!f2.IsValid());
        REQUIRE(!f3.IsValid());

        REQUIRE(f.IsValid());
        REQUIRE(!f.IsReady());

        std::move(p2).SetError(std::make_exception_ptr(std::runtime_error("second")));
        REQUIRE(!f.IsReady());

        std::move(p1).SetError(std::make_exception_ptr(std::runtime_error("first")));
        REQUIRE(!f.IsReady());

        std::move(p3).SetError(std::make_exception_ptr(std::runtime_error("third")));
        REQUIRE(f.IsReady());

        REQUIRE_THROWS_WITH(f.GetValue(), "third");
    });
}
