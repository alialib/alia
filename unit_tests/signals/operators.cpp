#include <alia/signals/basic.hpp>
#include <alia/signals/operators.hpp>
#include <alia/signals/utilities.hpp>
#include <string>

#include <catch.hpp>

TEST_CASE("signal operators", "[signals]")
{
    using namespace alia;

    REQUIRE(is_true(constant(2) == constant(2)));
    REQUIRE(is_false(constant(6) == constant(2)));
    REQUIRE(is_true(constant(6) != constant(2)));
    REQUIRE(is_false(constant(2) != constant(2)));
    REQUIRE(is_true(constant(6) > constant(2)));
    REQUIRE(is_false(constant(6) < constant(2)));
    REQUIRE(is_true(constant(6) >= constant(2)));
    REQUIRE(is_true(constant(2) >= constant(2)));
    REQUIRE(is_false(constant(2) >= constant(6)));
    REQUIRE(is_true(constant(2) < constant(6)));
    REQUIRE(is_false(constant(6) < constant(2)));
    REQUIRE(is_true(constant(2) <= constant(6)));
    REQUIRE(is_true(constant(2) <= constant(2)));
    REQUIRE(is_false(constant(6) <= constant(2)));

    REQUIRE(is_true(constant(6) + constant(2) == constant(8)));
    REQUIRE(is_true(constant(6) - constant(2) == constant(4)));
    REQUIRE(is_true(constant(6) * constant(2) == constant(12)));
    REQUIRE(is_true(constant(6) / constant(2) == constant(3)));
    REQUIRE(is_true(constant(6) % constant(2) == constant(0)));
    REQUIRE(is_true((constant(6) ^ constant(2)) == constant(4)));
    REQUIRE(is_true((constant(6) & constant(2)) == constant(2)));
    REQUIRE(is_true((constant(6) | constant(2)) == constant(6)));
    REQUIRE(is_true(constant(6) << constant(2) == constant(24)));
    REQUIRE(is_true(constant(6) >> constant(2) == constant(1)));

    REQUIRE(is_true(-constant(2) == constant(-2)));
    REQUIRE(is_false(!(constant(2) == constant(2))));
}
