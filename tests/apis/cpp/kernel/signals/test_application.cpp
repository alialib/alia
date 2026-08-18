#include <alia/kernel/signals/application.hpp>

#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::operators;

TEST_CASE("lazy_bidirectional_apply")
{
    int n = 0;
    auto f = [](int x) -> double { return x * 2.0; };
    auto r = [](double x) -> int { return int(x / 2.0 + 0.5); };
    auto s = lazy_bidirectional_apply(f, r, ref(n));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);
    static_assert(signal_with<decltype(s), view_caps<signal_move_activated>>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 0.0);
    CHECK(signal_ready_to_write(s));

    auto id_before = s.value_id();
    write_signal(s, 4.0);
    CHECK(n == 2);
    CHECK(read_signal(s) == 4.0);
    CHECK((s.value_id() != id_before));

    write_signal(s, 2.0);
    CHECK(n == 1);
    CHECK(read_signal(s) == 2.0);
}

TEST_CASE("lazy_bidirectional_apply with empty arg")
{
    auto f = [](int x) -> double { return x * 2.0; };
    auto r = [](double x) -> int { return int(x / 2.0 + 0.5); };
    auto s = lazy_bidirectional_apply(f, r, empty<int>());

    CHECK_FALSE(signal_has_value(s));
    CHECK_FALSE(signal_ready_to_write(s));
}
