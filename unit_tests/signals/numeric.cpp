#include <alia/signals/numeric.hpp>

#include <catch2/catch.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/utilities.hpp>

using namespace alia;

TEST_CASE("offset signal", "[signals]")
{
    double x = 1;
    auto s = offset(direct(x), value(0.5));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1.5);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 4);
    REQUIRE(x == 3.5);
}

TEST_CASE("scaled signal", "[signals]")
{
    double x = 1;
    auto s = scale(direct(x), value(0.5));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 0.5);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 2);
    REQUIRE(x == 4);
}

TEST_CASE("round_signal_writes", "[signals]")
{
    double x = 1;
    auto s = round_signal_writes(direct(x), value(0.5));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 0.4);
    REQUIRE(x == 0.5);
}
