#include <alia/core/signals/containers.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace alia;

TEST_CASE("container size query", "[signals][adaptors]")
{
    std::string x = "foob";
    auto wrapped = direct(x);
    auto s = size(wrapped);

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 4);
}

TEST_CASE("is_empty on empty string", "[signals][adaptors]")
{
    std::string x;
    auto wrapped = direct(x);
    auto s = is_empty(wrapped);

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == true);
}

TEST_CASE("is_empty on non-empty string", "[signals][adaptors]")
{
    std::string x = "foo";
    auto wrapped = direct(x);
    auto s = is_empty(wrapped);

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == false);
}

TEST_CASE("is_empty on empty signal", "[signals][adaptors]")
{
    auto s = is_empty(empty<std::string>());

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
}

TEST_CASE("hide_if_empty (hidden)", "[signals][adaptors]")
{
    std::string x;
    auto wrapped = direct(x);
    auto s = hide_if_empty(wrapped);

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
}

TEST_CASE("hide_if_empty (not hidden)", "[signals][adaptors]")
{
    std::string x("foo");
    auto wrapped = direct(x);
    auto s = hide_if_empty(wrapped);

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == "foo");
}
