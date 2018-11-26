#include <alia/signals/core.hpp>

#include <catch2/catch.hpp>

#include <alia/signals/basic.hpp>

using namespace alia;

TEST_CASE("signal_direction_is_compatible", "[signals]")
{
#define TEST_COMPATIBILITY(Expected, Actual, result)                           \
    REQUIRE(                                                                   \
        (signal_direction_is_compatible<Expected, Actual>::value) == (result))

    TEST_COMPATIBILITY(read_only_signal, read_only_signal, true);
    TEST_COMPATIBILITY(read_only_signal, write_only_signal, false);
    TEST_COMPATIBILITY(read_only_signal, bidirectional_signal, true);
    TEST_COMPATIBILITY(write_only_signal, read_only_signal, false);
    TEST_COMPATIBILITY(write_only_signal, write_only_signal, true);
    TEST_COMPATIBILITY(write_only_signal, bidirectional_signal, true);
    TEST_COMPATIBILITY(bidirectional_signal, read_only_signal, false);
    TEST_COMPATIBILITY(bidirectional_signal, write_only_signal, false);
    TEST_COMPATIBILITY(bidirectional_signal, bidirectional_signal, true);
}

TEST_CASE("signal_direction_intersection", "[signals]")
{
#define TEST_INTERSECTION(A, B, Result)                                        \
    REQUIRE((std::is_same<signal_direction_intersection<A, B>::type, Result>:: \
                 value))

    TEST_INTERSECTION(read_only_signal, read_only_signal, read_only_signal);
    TEST_INTERSECTION(read_only_signal, bidirectional_signal, read_only_signal);
    TEST_INTERSECTION(write_only_signal, write_only_signal, write_only_signal);
    TEST_INTERSECTION(
        write_only_signal, bidirectional_signal, write_only_signal);
    TEST_INTERSECTION(bidirectional_signal, read_only_signal, read_only_signal);
    TEST_INTERSECTION(
        bidirectional_signal, write_only_signal, write_only_signal);
    TEST_INTERSECTION(
        bidirectional_signal, bidirectional_signal, bidirectional_signal);
}

TEST_CASE("signal_direction_union", "[signals]")
{
#define TEST_UNION(A, B, Result)                                               \
    REQUIRE((std::is_same<signal_direction_union<A, B>::type, Result>::value))

    TEST_UNION(read_only_signal, read_only_signal, read_only_signal);
    TEST_UNION(read_only_signal, write_only_signal, bidirectional_signal);
    TEST_UNION(read_only_signal, bidirectional_signal, bidirectional_signal);
    TEST_UNION(write_only_signal, write_only_signal, write_only_signal);
    TEST_UNION(write_only_signal, read_only_signal, bidirectional_signal);
    TEST_UNION(write_only_signal, bidirectional_signal, bidirectional_signal);
    TEST_UNION(bidirectional_signal, read_only_signal, bidirectional_signal);
    TEST_UNION(bidirectional_signal, write_only_signal, bidirectional_signal);
    TEST_UNION(
        bidirectional_signal, bidirectional_signal, bidirectional_signal);
}

TEST_CASE("is_signal_type", "[signals]")
{
    REQUIRE(is_signal_type<input<int>>::value);
    REQUIRE(is_signal_type<output<int>>::value);
    REQUIRE(is_signal_type<bidirectional<int>>::value);
    REQUIRE(!is_signal_type<int>::value);
    REQUIRE(!is_signal_type<std::string>::value);
}

TEST_CASE("signal_can_read", "[signals]")
{
    REQUIRE(signal_can_read<input<int>>::value);
    REQUIRE(!signal_can_read<output<int>>::value);
    REQUIRE(signal_can_read<bidirectional<int>>::value);
}

TEST_CASE("is_readable_signal_type", "[signals]")
{
    REQUIRE(is_readable_signal_type<input<int>>::value);
    REQUIRE(!is_readable_signal_type<output<int>>::value);
    REQUIRE(is_readable_signal_type<bidirectional<int>>::value);
    REQUIRE(!is_readable_signal_type<int>::value);
    REQUIRE(!is_readable_signal_type<std::string>::value);
}

TEST_CASE("signal_can_write", "[signals]")
{
    REQUIRE(!signal_can_write<input<int>>::value);
    REQUIRE(signal_can_write<output<int>>::value);
    REQUIRE(signal_can_write<bidirectional<int>>::value);
}

TEST_CASE("is_writable_signal_type", "[signals]")
{
    REQUIRE(!is_writable_signal_type<input<int>>::value);
    REQUIRE(is_writable_signal_type<output<int>>::value);
    REQUIRE(is_writable_signal_type<bidirectional<int>>::value);
    REQUIRE(!is_writable_signal_type<int>::value);
    REQUIRE(!is_writable_signal_type<std::string>::value);
}

TEST_CASE("signal_ref", "[signals]")
{
    int x = 1;
    auto y = direct(x);
    signal_ref<int, bidirectional_signal> s = y;

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(s.value_id() == y.value_id());
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 0);
    REQUIRE(x == 0);
    REQUIRE(read_signal(s) == 0);
}

static void
f_input(alia::input<int> x)
{
}

static void
f_output(alia::output<int> x)
{
}

static void
f_bidirectional(alia::bidirectional<int> x)
{
}

TEST_CASE("signal parameter passing", "[signals]")
{
    auto read_only = value(0);
    int x = 0;
    auto bidirectional = direct(x);

    f_input(read_only);
    f_input(bidirectional);
    f_output(bidirectional);
    f_bidirectional(bidirectional);
}
