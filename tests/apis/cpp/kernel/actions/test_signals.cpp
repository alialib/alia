#include <alia/kernel/actions/signals.hpp>

#include <alia/kernel/actions/basic.hpp>
#include <alia/kernel/signals/basic.hpp>

#include <alia/test/kernel/effect_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::test;

TEST_CASE("add_write_action")
{
    effect_fixture fx;
    int x = 0;
    bool written = false;
    auto s
        = add_write_action(ref(x), callback([&](int) { written = true; }));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK_FALSE(written);
    CHECK(signal_ready_to_write(s));
    write_signal(&fx.ctx, s, 1);
    fx.run();
    CHECK(x == 1);
    CHECK(written);
}

TEST_CASE("unready add_write_action")
{
    int x = 0;
    auto s = add_write_action(ref(x), actions::unready<int>());

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK_FALSE(signal_ready_to_write(s));
}
