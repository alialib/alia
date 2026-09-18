#include <alia/kernel/actions/basic.hpp>

#include <alia/test/kernel/effect_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::test;

TEST_CASE("unready action")
{
    CHECK_FALSE(actions::unready().is_ready());
    CHECK_FALSE(actions::unready<int>().is_ready());
}

TEST_CASE("noop action")
{
    effect_fixture fx;
    CHECK(actions::noop().is_ready());
    post_action(&fx.ctx, actions::noop());
    fx.run();
    CHECK(actions::noop<int>().is_ready());
    post_action(&fx.ctx, actions::noop<int>(), 1);
    fx.run();
}

TEST_CASE("callbacks")
{
    effect_fixture fx;
    int x = 0;
    auto a = callback([&](int y, int z) { x = y + z; });
    post_action(&fx.ctx, a, 1, 2);
    fx.run();
    CHECK(x == 3);

    bool ready = false;
    auto b = callback([&]() { return ready; }, [&](int y) { x += y; });
    CHECK_FALSE(b.is_ready());
    ready = true;
    CHECK(b.is_ready());
    post_action(&fx.ctx, b, 1);
    fx.run();
    CHECK(x == 4);
}
