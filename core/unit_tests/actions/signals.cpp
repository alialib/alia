#include <alia/core/actions/signals.hpp>

#include <alia/core/actions/basic.hpp>
#include <alia/core/signals/basic.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace alia;

TEST_CASE("add_write_action", "[actions][signals]")
{
    int x = 0;
    bool written = false;
    auto s
        = add_write_action(direct(x), callback([&](int) { written = true; }));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(written == false);
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 1);
    REQUIRE(x == 1);
    REQUIRE(written == true);
}

TEST_CASE("unready add_write_action", "[actions][signals]")
{
    int x = 0;
    auto s = add_write_action(direct(x), unready_action<int>());

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(!signal_ready_to_write(s));
}
