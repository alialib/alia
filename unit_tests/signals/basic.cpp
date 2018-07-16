#include <alia/signals/basic.hpp>

#include <catch.hpp>

TEST_CASE("empty signal", "[signals]")
{
    using namespace alia;

    auto s = empty<int>();

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(s.value_id() == no_id);
    REQUIRE(!signal_is_readable(s));
    REQUIRE(!signal_is_writable(s));
}

TEST_CASE("value signal", "[signals]")
{
    using namespace alia;

    auto s = value(1);

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);

    // Test that signals with different values also have different IDs.
    // (This is what differentiates value signals from constant signals.)
    REQUIRE(s.value_id() != value(2).value_id());
}

TEST_CASE("direct signal", "[signals]")
{
    using namespace alia;

    int x = 1;
    auto s = direct(x);

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 0);
    REQUIRE(x == 0);
    REQUIRE(read_signal(s) == 0);
}

TEST_CASE("text signal", "[signals]")
{
    using namespace alia;

    auto s = text("hello");

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == std::string("hello"));
    // There aren't really any interesting requirements on this.
    REQUIRE(s.value_id() == s.value_id());
}
