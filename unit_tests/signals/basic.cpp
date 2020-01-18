#include <alia/signals/basic.hpp>

#include <catch.hpp>

using namespace alia;

TEST_CASE("empty signal", "[signals][basic]")
{
    auto s = empty<int>();

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(s.value_id() == no_id);
    REQUIRE(!signal_is_readable(s));
    REQUIRE(!signal_is_writable(s));
}

TEST_CASE("value signal", "[signals][basic]")
{
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

TEST_CASE("direct signal", "[signals][basic]")
{
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

TEST_CASE("direct const signal", "[signals][basic]")
{
    int x = 1;
    auto s = direct(static_cast<int const&>(x));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
}

TEST_CASE("string literal signal", "[signals][basic]")
{
    auto s = value("hello");

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == std::string("hello"));
    // There aren't really any interesting requirements on this.
    REQUIRE(s.value_id() == s.value_id());
}

TEST_CASE("string literal operator", "[signals][basic]")
{
    using namespace alia::literals;

    auto s = "hello"_a;

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == std::string("hello"));
    // There aren't really any interesting requirements on this.
    REQUIRE(s.value_id() == s.value_id());
}

TEST_CASE("integer literal operator", "[signals][basic]")
{
    using namespace alia::literals;

    auto s = 1_a;

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
}

TEST_CASE("float literal operator", "[signals][basic]")
{
    using namespace alia::literals;

    auto s = 1.5_a;

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1.5);
}
