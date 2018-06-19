#include <alia/signals/basic.hpp>
#include <string>

#include <catch.hpp>

TEST_CASE("empty signal", "[signals]")
{
    using namespace alia;

    auto s = empty_signal<int>();

    REQUIRE(signal_can_read<decltype(s)>::value);
    REQUIRE(signal_can_write<decltype(s)>::value);

    REQUIRE(!signal_is_readable(s));
    REQUIRE(!signal_is_writable(s));
}

TEST_CASE("constant signal", "[signals]")
{
    using namespace alia;

    typedef decltype(constant(1)) constant_signal_t;
    REQUIRE(signal_can_read<constant_signal_t>::value);
    REQUIRE(!signal_can_write<constant_signal_t>::value);

    REQUIRE(signal_is_readable(constant(1)));
    REQUIRE(read_signal(constant(1)) == 1);
}

TEST_CASE("value signal", "[signals]")
{
    using namespace alia;

    typedef decltype(value(1)) value_signal_t;
    REQUIRE(signal_can_read<value_signal_t>::value);
    REQUIRE(!signal_can_write<value_signal_t>::value);

    REQUIRE(signal_is_readable(value(1)));
    REQUIRE(read_signal(value(1)) == 1);

    // Test that signals with different values also have different IDs.
    // (This is what differentiates value signals from constant signals.)
    REQUIRE(value(1).value_id() != value(2).value_id());
}

TEST_CASE("direct inout signal", "[signals]")
{
    using namespace alia;

    int x = 1;
    auto s = direct_inout(&x);

    typedef decltype(s) value_signal_t;
    REQUIRE(signal_can_read<value_signal_t>::value);
    REQUIRE(signal_can_write<value_signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 0);
    REQUIRE(x == 0);
    REQUIRE(read_signal(s) == 0);
}
