#include <alia/signals/containers.hpp>

#include <testing.hpp>

using namespace alia;

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
