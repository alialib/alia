#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/core.hpp>

#include <doctest/doctest.h>

#include <string>

using namespace alia;
using namespace alia::operators;

TEST_CASE("signal_capabilities_compatible")
{
#define TEST_COMPATIBILITY(Expected, Actual, result)                          \
    CHECK((signal_capabilities_compatible<Expected, Actual>) == (result))

    TEST_COMPATIBILITY(
        view_caps<signal_readable>, view_caps<signal_readable>, true);
    TEST_COMPATIBILITY(
        view_caps<signal_readable>, sink_caps<signal_writable>, false);
    TEST_COMPATIBILITY(
        view_caps<signal_readable>, binding_caps<signal_readable>, true);
    TEST_COMPATIBILITY(
        view_caps<signal_readable>, view_caps<signal_movable>, true);
    TEST_COMPATIBILITY(
        view_caps<signal_readable>, binding_caps<signal_move_activated>, true);
    TEST_COMPATIBILITY(
        view_caps<signal_movable>, view_caps<signal_readable>, false);
    TEST_COMPATIBILITY(
        view_caps<signal_movable>, view_caps<signal_move_activated>, true);
    TEST_COMPATIBILITY(
        view_caps<signal_move_activated>, view_caps<signal_movable>, false);
    TEST_COMPATIBILITY(
        view_caps<signal_move_activated>, binding_caps<signal_movable>, false);
    TEST_COMPATIBILITY(
        sink_caps<signal_writable>, view_caps<signal_readable>, false);
    TEST_COMPATIBILITY(
        sink_caps<signal_writable>, sink_caps<signal_writable>, true);
    TEST_COMPATIBILITY(
        sink_caps<signal_writable>, binding_caps<signal_readable>, true);
    TEST_COMPATIBILITY(
        sink_caps<signal_writable>, view_caps<signal_movable>, false);
    TEST_COMPATIBILITY(
        sink_caps<signal_writable>, binding_caps<signal_move_activated>, true);
    TEST_COMPATIBILITY(
        binding_caps<signal_readable>, view_caps<signal_readable>, false);
    TEST_COMPATIBILITY(
        binding_caps<signal_readable>, sink_caps<signal_writable>, false);
    TEST_COMPATIBILITY(
        binding_caps<signal_readable>, binding_caps<signal_readable>, true);
    TEST_COMPATIBILITY(
        binding_caps<signal_readable>, view_caps<signal_movable>, false);
    TEST_COMPATIBILITY(
        binding_caps<signal_readable>,
        binding_caps<signal_move_activated>,
        true);
    TEST_COMPATIBILITY(
        binding_caps<signal_movable>, binding_caps<signal_movable>, true);
    TEST_COMPATIBILITY(
        binding_caps<signal_move_activated>,
        binding_caps<signal_movable>,
        false);
    TEST_COMPATIBILITY(
        binding_caps<signal_movable>,
        binding_caps<signal_move_activated>,
        true);
    TEST_COMPATIBILITY(
        binding_caps<signal_move_activated>,
        binding_caps<signal_movable>,
        false);
#undef TEST_COMPATIBILITY
}

TEST_CASE("signal_capabilities_intersection")
{
#define TEST_INTERSECTION(A, B, Result)                                       \
    CHECK((std::is_same_v<signal_capabilities_intersection<A, B>, Result>) )

    TEST_INTERSECTION(
        view_caps<signal_readable>,
        view_caps<signal_readable>,
        view_caps<signal_readable>);
    TEST_INTERSECTION(
        view_caps<signal_readable>,
        view_caps<signal_movable>,
        view_caps<signal_readable>);
    TEST_INTERSECTION(
        view_caps<signal_readable>,
        view_caps<signal_move_activated>,
        view_caps<signal_readable>);
    TEST_INTERSECTION(
        view_caps<signal_movable>,
        view_caps<signal_move_activated>,
        view_caps<signal_movable>);
    TEST_INTERSECTION(
        view_caps<signal_readable>,
        binding_caps<signal_readable>,
        view_caps<signal_readable>);
    TEST_INTERSECTION(
        sink_caps<signal_writable>,
        sink_caps<signal_writable>,
        sink_caps<signal_writable>);
    TEST_INTERSECTION(
        sink_caps<signal_writable>,
        binding_caps<signal_readable>,
        sink_caps<signal_writable>);
    TEST_INTERSECTION(
        binding_caps<signal_readable>,
        view_caps<signal_readable>,
        view_caps<signal_readable>);
    TEST_INTERSECTION(
        binding_caps<signal_readable>,
        sink_caps<signal_writable>,
        sink_caps<signal_writable>);
    TEST_INTERSECTION(
        binding_caps<signal_readable>,
        binding_caps<signal_readable>,
        binding_caps<signal_readable>);
    TEST_INTERSECTION(
        binding_caps<signal_movable>,
        binding_caps<signal_move_activated>,
        binding_caps<signal_movable>);
    TEST_INTERSECTION(
        binding_caps<signal_movable>,
        view_caps<signal_move_activated>,
        view_caps<signal_movable>);
#undef TEST_INTERSECTION
}

TEST_CASE("signal_capabilities_union")
{
#define TEST_UNION(A, B, Result)                                              \
    CHECK((std::is_same_v<signal_capabilities_union<A, B>, Result>) )

    TEST_UNION(
        view_caps<signal_readable>,
        view_caps<signal_readable>,
        view_caps<signal_readable>);
    TEST_UNION(
        view_caps<signal_readable>,
        view_caps<signal_movable>,
        view_caps<signal_movable>);
    TEST_UNION(
        view_caps<signal_readable>,
        view_caps<signal_move_activated>,
        view_caps<signal_move_activated>);
    TEST_UNION(
        view_caps<signal_movable>,
        view_caps<signal_move_activated>,
        view_caps<signal_move_activated>);
    TEST_UNION(
        view_caps<signal_readable>,
        sink_caps<signal_writable>,
        binding_caps<signal_readable>);
    TEST_UNION(
        view_caps<signal_readable>,
        binding_caps<signal_readable>,
        binding_caps<signal_readable>);
    TEST_UNION(
        sink_caps<signal_writable>,
        sink_caps<signal_writable>,
        sink_caps<signal_writable>);
    TEST_UNION(
        sink_caps<signal_writable>,
        view_caps<signal_readable>,
        binding_caps<signal_readable>);
    TEST_UNION(
        sink_caps<signal_writable>,
        binding_caps<signal_readable>,
        binding_caps<signal_readable>);
    TEST_UNION(
        binding_caps<signal_readable>,
        view_caps<signal_readable>,
        binding_caps<signal_readable>);
    TEST_UNION(
        binding_caps<signal_readable>,
        sink_caps<signal_writable>,
        binding_caps<signal_readable>);
    TEST_UNION(
        binding_caps<signal_readable>,
        binding_caps<signal_readable>,
        binding_caps<signal_readable>);
    TEST_UNION(
        binding_caps<signal_movable>,
        view_caps<signal_move_activated>,
        binding_caps<signal_move_activated>);
#undef TEST_UNION
}

TEST_CASE("signal_type")
{
    CHECK(signal_type<view<int>>);
    CHECK(signal_type<sink<int>>);
    CHECK(signal_type<binding<int>>);
    CHECK_FALSE(signal_type<int>);
    CHECK_FALSE(signal_type<std::string>);
}

TEST_CASE("view_signal")
{
    CHECK(view_signal<view<int>>);
    CHECK_FALSE(view_signal<sink<int>>);
    CHECK(view_signal<binding<int>>);
    CHECK_FALSE(view_signal<int>);
    CHECK_FALSE(view_signal<std::string>);
}

TEST_CASE("sink_signal")
{
    CHECK_FALSE(sink_signal<view<int>>);
    CHECK(sink_signal<sink<int>>);
    CHECK(sink_signal<binding<int>>);
    CHECK_FALSE(sink_signal<int>);
    CHECK_FALSE(sink_signal<std::string>);
}

TEST_CASE("binding_signal")
{
    CHECK_FALSE(binding_signal<view<int>>);
    CHECK_FALSE(binding_signal<sink<int>>);
    CHECK(binding_signal<binding<int>>);
    CHECK(binding_signal<signal_ref<int, binding_caps<signal_movable>>>);
    CHECK_FALSE(
        binding_signal<signal_ref<int, binding_caps<signal_readable>>>);
    CHECK_FALSE(binding_signal<int>);
    CHECK_FALSE(binding_signal<std::string>);
}

TEST_CASE("view_of")
{
    CHECK(view_of<view<int>, int>);
    CHECK_FALSE(view_of<view<int>, float>);
    CHECK(view_of<binding<int>, int>);
    CHECK_FALSE(view_of<sink<int>, int>);
}

TEST_CASE("signal_with clearable sink")
{
    CHECK_FALSE(signal_with<view<int>, sink_caps<signal_clearable>>);
    CHECK_FALSE(signal_with<sink<int>, sink_caps<signal_clearable>>);
    CHECK_FALSE(signal_with<binding<int>, sink_caps<signal_clearable>>);
    CHECK(
        signal_with<
            signal_ref<int, binding_caps<signal_readable, signal_clearable>>,
            sink_caps<signal_clearable>>);
    CHECK_FALSE(signal_with<int, sink_caps<signal_clearable>>);
}

TEST_CASE("signal_ref")
{
    int x = 1;
    auto y = ref(x);
    signal_ref<int, binding_caps<signal_readable>> s = y;

    using signal_t = decltype(s);
    CHECK(view_signal<signal_t>);
    CHECK(sink_signal<signal_t>);
    CHECK_FALSE(binding_signal<signal_t>);

    CHECK(signal_has_value(s));
    CHECK((s.value_id() == to_id_view(y.value_id())));
    CHECK(read_signal(s) == 1);
    CHECK(signal_ready_to_write(s));
    write_signal(s, 0);
    CHECK(x == 0);
    CHECK(read_signal(s) == 0);
}

static void
f_view(alia::view<int>)
{
}

static void
f_sink(alia::sink<int>)
{
}

static void
f_binding(alia::binding<int>)
{
}

TEST_CASE("signal parameter passing")
{
    auto read_only = value(0);
    int x = 0;
    auto bidirectional = ref(x);

    f_view(read_only);
    f_view(bidirectional);
    f_sink(bidirectional);
    f_binding(bidirectional);
}
