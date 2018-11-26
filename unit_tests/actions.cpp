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

TEST_CASE("toggle action", "[actions]")
{
    bool x = false;
    {
        auto a = make_toggle_action(direct(x));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x);
    }
    {
        auto a = make_toggle_action(direct(x));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(!x);
    }

    {
        auto a = make_toggle_action(empty<bool>());
        REQUIRE(!a.is_ready());
    }
}

TEST_CASE("push_back action", "[actions]")
{
    auto x = std::vector<int>{1, 2};
    {
        auto a = make_push_back_action(direct(x), value(3));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x == std::vector<int>{1, 2, 3});
    }

    {
        auto a = make_push_back_action(direct(x), empty<int>());
        REQUIRE(!a.is_ready());
    }
    {
        auto a = make_push_back_action(empty<std::vector<int>>(), value(3));
        REQUIRE(!a.is_ready());
    }
}

TEST_CASE("lambda actions", "[actions]")
{
    int x = 0;
    auto a = lambda_action(always_ready, [&](int y, int z) { x = y + z; });
    perform_action(a, 1, 2);
    REQUIRE(x == 3);

    bool ready = false;
    auto b = lambda_action([&]() { return ready; }, [&](int y) { x += y; });
    REQUIRE(!b.is_ready());
    ready = true;
    REQUIRE(b.is_ready());
    perform_action(b, 1);
    REQUIRE(x == 4);
}
