#include <alia/kernel/signals/lambdas.hpp>

#include <doctest/doctest.h>

#include <vector>

using namespace alia;
using namespace alia::operators;

TEST_CASE("lambda_constant")
{
    int calls = 0;
    auto s = lambda_constant([&] {
        ++calls;
        return 1;
    });

    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);
    static_assert(signal_with<decltype(s), view_caps<signal_move_activated>>);

    CHECK(calls == 0);
    CHECK(signal_has_value(s));
    CHECK(std::is_same_v<decltype(s.value_id()), constant_value_tag>);
    CHECK(calls == 0);

    CHECK(read_signal(s) == 1);
    CHECK(calls == 1);
    CHECK(move_from_signal(s) == 1);
    CHECK(calls == 2);
}

TEST_CASE("lambda_constant of a non-identifiable value")
{
    auto s = lambda_constant([] { return std::vector<int>(3, 2); });

    CHECK(signal_has_value(s));
    CHECK(std::is_same_v<decltype(s.value_id()), constant_value_tag>);
    CHECK(read_signal(s) == std::vector<int>({2, 2, 2}));
}

TEST_CASE("lambda_view")
{
    int x = 1;
    auto s = lambda_view([&x] { return x; });

    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);
    static_assert(signal_with<decltype(s), view_caps<signal_move_activated>>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1);
    CHECK(move_from_signal(s) == 1);
    CHECK(s.value_id() == 1);

    x = 0;
    CHECK(read_signal(s) == 0);
    CHECK(s.value_id() == 0);
}

TEST_CASE("lambda_binding")
{
    int x = 1;
    auto s = lambda_binding([&x] { return x; }, [&x](int v) { x = v; });

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);
    static_assert(
        signal_with<decltype(s), binding_caps<signal_move_activated>>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1);
    CHECK(move_from_signal(s) == 1);
    CHECK(signal_ready_to_write(s));
    CHECK(s.value_id() == 1);

    auto id_before = s.value_id();
    write_signal(s, 0);
    CHECK(x == 0);
    CHECK(read_signal(s) == 0);
    CHECK(move_from_signal(s) == 0);
    CHECK(s.value_id() == 0);
    CHECK(s.value_id() != id_before);
}
