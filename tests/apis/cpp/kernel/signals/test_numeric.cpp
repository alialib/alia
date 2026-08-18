#include <alia/kernel/signals/numeric.hpp>

#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::operators;

TEST_CASE("offset")
{
    double x = 1;
    auto s = offset(ref(x), value(0.5));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);
    static_assert(signal_with<decltype(s), view_caps<signal_move_activated>>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1.5);

    auto id_before = s.value_id();
    CHECK((offset(ref(x), 0.5).value_id() == id_before));

    CHECK(signal_ready_to_write(s));
    write_signal(s, 4);
    CHECK(x == 3.5);
    CHECK((offset(ref(x), 0.5).value_id() != id_before));
}

TEST_CASE("scale")
{
    double x = 1;
    auto s = scale(ref(x), value(0.5));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);
    static_assert(signal_with<decltype(s), view_caps<signal_move_activated>>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 0.5);

    auto id_before = s.value_id();
    CHECK((scale(ref(x), 0.5).value_id() == id_before));

    CHECK(signal_ready_to_write(s));
    write_signal(s, 2);
    CHECK(x == 4);
    CHECK((scale(ref(x), 0.5).value_id() != id_before));
}

TEST_CASE("scale with empty factor")
{
    double x = 1;
    auto s = scale(ref(x), empty<double>());
    CHECK_FALSE(signal_has_value(s));
    CHECK_FALSE(signal_ready_to_write(s));
}

TEST_CASE("round_signal_writes")
{
    double x = 1;
    auto s = round_signal_writes(ref(x), value(0.5));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1);
    CHECK(signal_ready_to_write(s));
    write_signal(s, 0.4);
    CHECK(x == 0.5);
}
