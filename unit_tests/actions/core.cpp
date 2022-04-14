#include <alia/actions/core.hpp>

#include <alia/actions/operators.hpp>
#include <alia/signals/basic.hpp>

#include <testing.hpp>

using namespace alia;

TEST_CASE("latch-like action", "[actions][core]")
{
    // Test that actions are performed with latch-like semantics.
    int x = 2, y = 3;
    perform_action((direct(x) <<= 4, direct(y) <<= direct(x)));
    REQUIRE(x == 4);
    REQUIRE(y == 2);
}

TEST_CASE("action_ref", "[actions][core]")
{
    int x = 1;
    auto a = empty<int>() <<= empty<int>();
    auto b = direct(x) <<= value(2);

    action_ref<> r = b;
    REQUIRE(r.is_ready());
    perform_action(r);
    REQUIRE(x == 2);

    x = 1;

    action_ref<> s = r;
    REQUIRE(s.is_ready());
    REQUIRE(action_is_ready(s));
    perform_action(s);
    REQUIRE(x == 2);

    s = a;
    REQUIRE(!s.is_ready());
    REQUIRE(!action_is_ready(s));
}

static void
f(action<> a)
{
    REQUIRE(!a.is_ready());
}

TEST_CASE("action parameter passing", "[actions][core]")
{
    auto a = empty<int>() <<= empty<int>();
    f(a);
}
