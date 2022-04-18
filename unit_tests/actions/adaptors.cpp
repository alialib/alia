#include <alia/actions/adaptors.hpp>

#include <alia/actions/operators.hpp>
#include <alia/signals/basic.hpp>

#include <testing.hpp>

using namespace alia;

TEST_CASE("actionize an action", "[actions][adaptors]")
{
    int x = 0;
    auto a = actionize(direct(x) <<= 1);
    REQUIRE(a.is_ready());
    perform_action(a);
    REQUIRE(x == 1);
}

TEST_CASE("actionize a lambda", "[actions][adaptors]")
{
    int x = 0;
    auto a = actionize([&] { x = 1; });
    REQUIRE(a.is_ready());
    perform_action(a);
    REQUIRE(x == 1);
}

TEST_CASE("only_if_ready on an unready input action", "[actions][adaptors]")
{
    bool a_ran = false;
    auto a = callback([&] { a_ran = true; });

    int x = 0;
    auto b = direct(x) <<= empty<int>();
    REQUIRE(!b.is_ready());

    auto combined = (a, only_if_ready(b));
    REQUIRE(combined.is_ready());
    perform_action(combined);
    REQUIRE(a_ran);
}

TEST_CASE("only_if_ready on a ready input action", "[actions][adaptors]")
{
    bool a_ran = false;
    auto a = callback([&] { a_ran = true; });

    int x = 0;
    auto b = direct(x) <<= value(1);
    REQUIRE(b.is_ready());

    auto combined = (a, only_if_ready(b));
    REQUIRE(combined.is_ready());
    perform_action(combined);
    REQUIRE(a_ran);
    REQUIRE(x == 1);
}

TEST_CASE("mask an action", "[actions][adaptors]")
{
    bool a_ran = false;
    auto a = callback([&] { a_ran = true; });

    auto masked_off = mask(a, false);
    REQUIRE(!masked_off.is_ready());

    auto masked_on = mask(a, value(true));
    REQUIRE(masked_on.is_ready());
    perform_action(masked_on);
    REQUIRE(a_ran);
}
