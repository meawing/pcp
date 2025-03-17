#include <catch2/catch_all.hpp>

#include "simple-sum.hpp"

TEST_CASE("SumWorks") {
    REQUIRE(Sum(2, 2) == 4);
    REQUIRE(Sum(-1, 1) == 0);
    REQUIRE(Sum(0, 0) == 0);
    REQUIRE(Sum(1, 2) == Sum(2, 1));
}
