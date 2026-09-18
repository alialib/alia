#include <alia/kernel/actions/core.hpp>

#include <alia/kernel/actions/operators.hpp>
#include <alia/kernel/signals/basic.hpp>

#include <alia/test/kernel/effect_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::test;

TEST_CASE("latch-like action")
{
    effect_fixture fx;
    // Test that actions have latch-like semantics.
    int x = 2, y = 3;
    post_action(&fx.ctx, (ref(x) <<= ref(y), ref(y) <<= ref(x)));
    fx.run();
    CHECK(x == 3);
    CHECK(y == 2);
}

TEST_CASE("action_ref")
{
    effect_fixture fx;
    int x = 1;
    auto a = empty<int>() <<= empty<int>();
    auto b = ref(x) <<= value(2);

    action_ref<> r = b;
    CHECK(r.is_ready());
    post_action(&fx.ctx, r);
    fx.run();
    CHECK(x == 2);

    x = 1;

    action_ref<> s = r;
    CHECK(s.is_ready());
    CHECK(action_is_ready(s));
    post_action(&fx.ctx, s);
    fx.run();
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
