#include <alia/core/signals/basic.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace alia;

TEST_CASE("empty signal", "[signals][basic]")
{
    auto s = empty<int>();

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(s.value_id() == null_id);
    REQUIRE(!signal_has_value(s));
    REQUIRE(!signal_ready_to_write(s));
}

TEST_CASE("default-initialized signal", "[signals][basic]")
{
    auto s = default_initialized<std::string>();

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);

    REQUIRE(s.value_id() == unit_id);
    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == std::string());
    REQUIRE(s.destructive_ref() == std::string());
    REQUIRE(move_signal(s) == std::string());
}

TEST_CASE("value signal", "[signals][basic]")
{
    auto s = value(1);

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);
    REQUIRE(!signal_is_clearable<signal_t>::value);

    REQUIRE(signal_has_value(s));
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
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);
    REQUIRE(!signal_is_clearable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 0);
    REQUIRE(x == 0);
    REQUIRE(read_signal(s) == 0);
}

TEST_CASE("direct const signal", "[signals][basic]")
{
    int x = 1;
    auto s = direct(static_cast<int const&>(x));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);
    REQUIRE(!signal_is_clearable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
}

TEST_CASE("string literal signal", "[signals][basic]")
{
    auto s = value("hello");

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);
    REQUIRE(!signal_is_clearable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == std::string("hello"));
    // There aren't really any interesting requirements on this.
    REQUIRE(s.value_id() == s.value_id());
}

TEST_CASE("string literal operator", "[signals][basic]")
{
    using namespace alia::literals;

    auto s = "hello"_a;

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);
    REQUIRE(!signal_is_clearable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == std::string("hello"));
    // There aren't really any interesting requirements on this.
    REQUIRE(s.value_id() == s.value_id());
}
