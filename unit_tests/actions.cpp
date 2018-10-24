#include <alia/actions.hpp>

#include <catch2/catch.hpp>

using namespace alia;

TEST_CASE("copy actions", "[actions]")
{
    int x = 1;
    REQUIRE(!(empty<int>() <<= empty<int>()).is_ready());
    REQUIRE(!(direct(x) <<= empty<int>()).is_ready());
    REQUIRE(!(empty<int>() <<= direct(x)).is_ready());
    auto a = direct(x) <<= value(2);
    REQUIRE(a.is_ready());
    REQUIRE(x == 1);
    perform_action(a);
    REQUIRE(x == 2);
}

TEST_CASE("sequenced actions", "[actions]")
{
    int x = 1, y = 2;
    auto a = empty<int>() <<= empty<int>();
    auto b = direct(x) <<= value(2);
    auto c = direct(y) <<= value(3);
    REQUIRE(!(a, b).is_ready());
    REQUIRE((b, c).is_ready());
    perform_action((b, c));
    REQUIRE(x == 2);
    REQUIRE(y == 3);
}

TEST_CASE("action_ref", "[actions]")
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
    perform_action(s);
    REQUIRE(x == 2);

    s = a;
    REQUIRE(!s.is_ready());
}

static void
f(action<> a)
{
    REQUIRE(!a.is_ready());
}

TEST_CASE("action parameter passing", "[actions]")
{
    auto a = empty<int>() <<= empty<int>();
    f(a);
}
