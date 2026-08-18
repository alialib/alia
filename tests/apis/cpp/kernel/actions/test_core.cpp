#include <alia/kernel/actions/core.hpp>

#include <alia/kernel/actions/operators.hpp>
#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

using namespace alia;

TEST_CASE("latch-like action")
{
    // Test that actions are performed with latch-like semantics.
    int x = 2, y = 3;
    perform_action((ref(x) <<= 4, ref(y) <<= ref(x)));
    CHECK(x == 4);
    CHECK(y == 2);
}

TEST_CASE("action_ref")
{
    int x = 1;
    auto a = empty<int>() <<= empty<int>();
    auto b = ref(x) <<= value(2);

    action_ref<> r = b;
    CHECK(r.is_ready());
    perform_action(r);
    CHECK(x == 2);

    x = 1;

    action_ref<> s = r;
    CHECK(s.is_ready());
    CHECK(action_is_ready(s));
    perform_action(s);
    CHECK(x == 2);

    s = a;
    CHECK_FALSE(s.is_ready());
    CHECK_FALSE(action_is_ready(s));
}

static void
f(action<> a)
{
    CHECK_FALSE(a.is_ready());
}

TEST_CASE("action parameter passing")
{
    auto a = empty<int>() <<= empty<int>();
    f(a);
}
