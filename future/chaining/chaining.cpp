#include <catch2/catch_all.hpp>
#include <lines/util/random.hpp>
#include <lines/util/move_only.hpp>
#include <lines/fibers/api.hpp>

#include <future/future.hpp>

////////////////////////////////////////////////////////////////////////////////

struct MoveOnlyInt : public lines::MoveOnly {
    explicit MoveOnlyInt(int value) : value{value} {
    }

    int value = 1;
};

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("ThenValue") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();

        std::move(p).SetValue(1234);

        auto fres = std::move(f).ThenValue([](MoveOnlyInt value) -> std::string {
            REQUIRE(value.value == 1234);
            return "42";
        });

        REQUIRE(!f.IsValid());

        REQUIRE(std::move(fres).GetValue() == "42");
    });
}

TEST_CASE("ThenValueBadUsage") {
    lines::SchedulerRun([] {
        future::Future<MoveOnlyInt> f;
        REQUIRE_THROWS_AS(std::move(f).ThenValue([](auto) { return 42; }), future::FutureInvalid);
    });
}

TEST_CASE("ThenValueCatchesException") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto fres = std::move(f).ThenValue([](std::string&& str) -> std::string {  //
            throw std::runtime_error(std::move(str));
        });

        std::move(p).SetValue("oops");

        REQUIRE_THROWS_WITH(std::move(fres).GetValue(), "oops");
    });
}

TEST_CASE("ThenValueForwardsException") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();

        auto fres = std::move(f).ThenValue([](MoveOnlyInt res) -> std::string {  //
            REQUIRE(false);
            return "42";
        });

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));

        REQUIRE_THROWS_WITH(std::move(fres).GetValue(), "oops");
    });
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("ThenError") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));

        auto fres = std::move(f).ThenError([](std::exception_ptr e) -> MoveOnlyInt {
            try {
                std::rethrow_exception(e);
            } catch (std::runtime_error& e) {
                REQUIRE(e.what() == std::string("oops"));
            } catch (...) {
                REQUIRE(false);
            }
            return MoveOnlyInt{42};
        });

        REQUIRE(!f.IsValid());

        REQUIRE(std::move(fres).GetValue().value == 42);
    });
}

TEST_CASE("ThenErrorBadUsage") {
    lines::SchedulerRun([] {
        future::Future<MoveOnlyInt> f;
        REQUIRE_THROWS_AS(std::move(f).ThenError([](auto) { return 42; }), future::FutureInvalid);
    });
}

TEST_CASE("ThenErrorCatchesException") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto fres = std::move(f).ThenError([](std::exception_ptr e) -> std::string {  //
            throw std::runtime_error("inner");
        });

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));

        REQUIRE_THROWS_WITH(std::move(fres).GetValue(), "inner");
    });
}

TEST_CASE("ThenErrorForwardsValue") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto fres = std::move(f).ThenError([](std::exception_ptr e) -> std::string {
            REQUIRE(false);
            return "";
        });

        std::move(p).SetValue("success");

        REQUIRE(std::move(fres).GetValue() == "success");
    });
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("ThenValueAsync") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();
        auto [f1, p1] = future::GetTied<std::string>();

        auto fres = std::move(f).ThenValue([f1 = std::move(f1)](MoveOnlyInt res) mutable {
            REQUIRE(res.value == 42);
            return std::move(f1);
        });

        REQUIRE(!f.IsValid());

        std::move(p).SetValue(42);

        REQUIRE(!fres.IsReady());

        std::move(p1).SetValue("p1");

        REQUIRE(fres.IsReady());
        REQUIRE(fres.GetValue() == "p1");
    });
}

TEST_CASE("ThenValueAsyncBadUsage") {
    lines::SchedulerRun([] {
        future::Future<MoveOnlyInt> f;
        REQUIRE_THROWS_AS(std::move(f).ThenValue([](auto) { return future::Future<int>(); }),
                          future::FutureInvalid);
    });
}

TEST_CASE("ThenValueAsyncCatchesException") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();
        auto [f1, p1] = future::GetTied<int>();

        auto fres = std::move(f).ThenValue([f1 = std::move(f1)](std::string&& str) mutable {
            REQUIRE(str == "outer");
            throw std::runtime_error("oops");
            return std::move(f1);
        });

        REQUIRE(!fres.IsReady());

        std::move(p).SetValue("outer");

        REQUIRE(fres.IsReady());
        REQUIRE_THROWS_WITH(fres.GetValue(), "oops");
    });
}

TEST_CASE("ThenValueAsyncForwardsException") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto fres = std::move(f).ThenValue([](std::string&& str) mutable -> future::Future<int> {
            REQUIRE(false);
            return {};
        });

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));

        REQUIRE(fres.IsReady());
        REQUIRE_THROWS_WITH(fres.GetValue(), "oops");
    });
}

TEST_CASE("ThenValueAsyncInvalidFuture") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto fres = std::move(f).ThenValue([](std::string&& str) mutable -> future::Future<int> {
            REQUIRE(str == "outer");
            return {};
        });

        std::move(p).SetValue("outer");

        REQUIRE(fres.IsReady());
        REQUIRE_THROWS_AS(fres.GetValue(), future::FutureInvalid);
    });
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("ThenErrorAsync") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<MoveOnlyInt>();
        auto [f1, p1] = future::GetTied<MoveOnlyInt>();

        auto fres = std::move(f).ThenError([f1 = std::move(f1)](std::exception_ptr e) mutable {
            try {
                std::rethrow_exception(e);
            } catch (std::runtime_error& e) {
                REQUIRE(e.what() == std::string("oops"));
            } catch (...) {
                REQUIRE(false);
            }
            return std::move(f1);
        });

        REQUIRE(!f.IsValid());

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));

        REQUIRE(!fres.IsReady());

        std::move(p1).SetValue(42);

        REQUIRE(fres.IsReady());
        REQUIRE(fres.GetValue().value == 42);
    });
}

TEST_CASE("ThenErrorAsyncBadUsage") {
    lines::SchedulerRun([] {
        future::Future<MoveOnlyInt> f;
        REQUIRE_THROWS_AS(
            std::move(f).ThenError([](auto) { return future::Future<MoveOnlyInt>(); }),
            future::FutureInvalid);
    });
}

TEST_CASE("ThenErrorAsyncCatchesException") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto fres =
            std::move(f).ThenError([](std::exception_ptr e) mutable -> future::Future<std::string> {
                try {
                    std::rethrow_exception(e);
                } catch (std::runtime_error& e) {
                    REQUIRE(e.what() == std::string("oops"));
                } catch (...) {
                    REQUIRE(false);
                }
                throw std::runtime_error("inner");
            });

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));

        REQUIRE(fres.IsReady());
        REQUIRE_THROWS_WITH(fres.GetValue(), "inner");
    });
}

TEST_CASE("ThenErrorAsyncForwardsValue") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto fres =
            std::move(f).ThenError([](std::exception_ptr e) mutable -> future::Future<std::string> {
                REQUIRE(false);
                return {};
            });

        std::move(p).SetValue("42");

        REQUIRE(fres.IsReady());
        REQUIRE(fres.GetValue() == "42");
    });
}

TEST_CASE("ThenErrorAsyncInvalidFuture") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto fres =
            std::move(f).ThenError([](std::exception_ptr e) mutable -> future::Future<std::string> {
                try {
                    std::rethrow_exception(e);
                } catch (std::runtime_error& e) {
                    REQUIRE(e.what() == std::string("oops"));
                } catch (...) {
                    REQUIRE(false);
                }
                return {};
            });

        std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));

        REQUIRE(fres.IsReady());
        REQUIRE_THROWS_AS(fres.GetValue(), future::FutureInvalid);
    });
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE("MultithreadedSync") {
    lines::SchedulerRun([] {
        auto [f, p] = future::GetTied<std::string>();

        auto consumer = lines::Spawn([f = std::move(f)]() mutable {
            auto fres = std::move(f).ThenValue([](const std::string& str) -> int {
                REQUIRE(str == "result");
                return 42;
            });
            fres = std::move(fres).ThenError([](std::exception_ptr e) -> int {
                try {
                    std::rethrow_exception(e);
                } catch (std::runtime_error& e) {
                    REQUIRE(e.what() == std::string("oops"));
                } catch (...) {
                    REQUIRE(false);
                }
                return 42;
            });

            REQUIRE(fres.Wait().GetValue() == 42);
        });

        auto producer = lines::Spawn([p = std::move(p)]() mutable {
            if (lines::FlipCoin()) {
                std::move(p).SetValue("result");
            } else {
                std::move(p).SetError(std::make_exception_ptr(std::runtime_error("oops")));
            }
        });

        consumer.join();
        producer.join();
    });
}

TEST_CASE("MultithreadedAsync") {
    lines::SchedulerRun(
        [] {
            auto [f, p] = future::GetTied<std::string>();

            auto [f1, p1] = future::GetTied<std::string>();
            auto [f2, p2] = future::GetTied<std::string>();

            const bool p_throws = lines::FlipCoin();
            const bool p1_throws = lines::FlipCoin();
            const bool p2_throws = lines::FlipCoin();

            auto consumer = lines::Spawn([&]() mutable {
                f = std::move(f).ThenValue([&](const std::string& str) mutable {
                    REQUIRE(!p_throws);
                    REQUIRE(str == "p_value");
                    return std::move(f1);
                });

                f = std::move(f).ThenError([&](std::exception_ptr e) mutable {
                    try {
                        std::rethrow_exception(e);
                    } catch (std::runtime_error& e) {
                        std::string str(e.what());
                        if (p_throws) {
                            REQUIRE(str == "p_error");
                        } else {
                            REQUIRE(str == "p1_error");
                        }
                    } catch (...) {
                        REQUIRE(false);
                    }
                    return std::move(f2);
                });

                f.Wait();
                if (f.HasException()) {
                    REQUIRE(p2_throws);
                    REQUIRE_THROWS_WITH(f.GetValue(), "p2_error");
                } else {
                    REQUIRE((!p1_throws || !p2_throws));
                    if (p_throws || p1_throws) {
                        REQUIRE(f.GetValue() == "p2_value");
                    } else {
                        REQUIRE(f.GetValue() == "p1_value");
                    }
                }
            });

            auto producer = lines::Spawn([&]() mutable {
                if (!p_throws) {
                    std::move(p).SetValue("p_value");
                } else {
                    std::move(p).SetError(std::make_exception_ptr(std::runtime_error("p_error")));
                }
            });

            auto producer1 = lines::Spawn([&]() mutable {
                if (!p1_throws) {
                    std::move(p1).SetValue("p1_value");
                } else {
                    std::move(p1).SetError(std::make_exception_ptr(std::runtime_error("p1_error")));
                }
            });

            auto producer2 = lines::Spawn([&]() mutable {
                if (!p2_throws) {
                    std::move(p2).SetValue("p2_value");
                } else {
                    std::move(p2).SetError(std::make_exception_ptr(std::runtime_error("p2_error")));
                }
            });

            consumer.join();
            producer.join();
            producer1.join();
            producer2.join();
        },
        100);
}
