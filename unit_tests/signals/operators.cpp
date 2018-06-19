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

TEST_CASE("select_signal", "[signals]")
{
    using namespace alia;

    bool condition = false;
    auto s = select_signal(direct(&condition), constant(1), constant(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
}

TEST_CASE("select_signal with different directions", "[signals]")
{
    using namespace alia;

    bool condition = false;
    auto s = select_signal(direct(&condition), empty<int>(), constant(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(!signal_is_readable(s));
}
