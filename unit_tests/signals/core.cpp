#include <alia/signals/core.hpp>

#include <catch.hpp>

TEST_CASE("signal_direction_is_compatible", "[signals]")
{
    using namespace alia;

#define TEST_COMPATIBILITY(Expected, Actual, result)                           \
    REQUIRE(                                                                   \
        (signal_direction_is_compatible<Expected, Actual>::value) == (result))

    TEST_COMPATIBILITY(read_only_signal, read_only_signal, true);
    TEST_COMPATIBILITY(read_only_signal, write_only_signal, false);
    TEST_COMPATIBILITY(read_only_signal, two_way_signal, true);
    TEST_COMPATIBILITY(write_only_signal, read_only_signal, false);
    TEST_COMPATIBILITY(write_only_signal, write_only_signal, true);
    TEST_COMPATIBILITY(write_only_signal, two_way_signal, true);
    TEST_COMPATIBILITY(two_way_signal, read_only_signal, false);
    TEST_COMPATIBILITY(two_way_signal, write_only_signal, false);
    TEST_COMPATIBILITY(two_way_signal, two_way_signal, true);
}

TEST_CASE("signal_direction_intersection", "[signals]")
{
    using namespace alia;

#define TEST_INTERSECTION(A, B, Result)                                        \
    REQUIRE((std::is_same<signal_direction_intersection<A, B>::type, Result>:: \
                 value))

    TEST_INTERSECTION(read_only_signal, read_only_signal, read_only_signal);
    TEST_INTERSECTION(read_only_signal, two_way_signal, read_only_signal);
    TEST_INTERSECTION(write_only_signal, write_only_signal, write_only_signal);
    TEST_INTERSECTION(write_only_signal, two_way_signal, write_only_signal);
    TEST_INTERSECTION(two_way_signal, read_only_signal, read_only_signal);
    TEST_INTERSECTION(two_way_signal, write_only_signal, write_only_signal);
    TEST_INTERSECTION(two_way_signal, two_way_signal, two_way_signal);
}

TEST_CASE("signal_direction_union", "[signals]")
{
    using namespace alia;

#define TEST_UNION(A, B, Result)                                               \
    REQUIRE((std::is_same<signal_direction_union<A, B>::type, Result>::value))

    TEST_UNION(read_only_signal, read_only_signal, read_only_signal);
    TEST_UNION(read_only_signal, write_only_signal, two_way_signal);
    TEST_UNION(read_only_signal, two_way_signal, two_way_signal);
    TEST_UNION(write_only_signal, write_only_signal, write_only_signal);
    TEST_UNION(write_only_signal, read_only_signal, two_way_signal);
    TEST_UNION(write_only_signal, two_way_signal, two_way_signal);
    TEST_UNION(two_way_signal, read_only_signal, two_way_signal);
    TEST_UNION(two_way_signal, write_only_signal, two_way_signal);
    TEST_UNION(two_way_signal, two_way_signal, two_way_signal);
}

TEST_CASE("is_signal_type", "[signals]")
{
    using namespace alia;

    REQUIRE(is_signal_type<input<int>>::value);
    REQUIRE(is_signal_type<output<int>>::value);
    REQUIRE(is_signal_type<inout<int>>::value);
    REQUIRE(!is_signal_type<int>::value);
    REQUIRE(!is_signal_type<std::string>::value);
}

TEST_CASE("signal_can_read", "[signals]")
{
    using namespace alia;

    REQUIRE(signal_can_read<input<int>>::value);
    REQUIRE(!signal_can_read<output<int>>::value);
    REQUIRE(signal_can_read<inout<int>>::value);
}

TEST_CASE("is_readable_signal_type", "[signals]")
{
    using namespace alia;

    REQUIRE(is_readable_signal_type<input<int>>::value);
    REQUIRE(!is_readable_signal_type<output<int>>::value);
    REQUIRE(is_readable_signal_type<inout<int>>::value);
    REQUIRE(!is_readable_signal_type<int>::value);
    REQUIRE(!is_readable_signal_type<std::string>::value);
}

TEST_CASE("signal_can_write", "[signals]")
{
    using namespace alia;

    REQUIRE(!signal_can_write<input<int>>::value);
    REQUIRE(signal_can_write<output<int>>::value);
    REQUIRE(signal_can_write<inout<int>>::value);
}

TEST_CASE("is_writable_signal_type", "[signals]")
{
    using namespace alia;

    REQUIRE(!is_writable_signal_type<input<int>>::value);
    REQUIRE(is_writable_signal_type<output<int>>::value);
    REQUIRE(is_writable_signal_type<inout<int>>::value);
    REQUIRE(!is_writable_signal_type<int>::value);
    REQUIRE(!is_writable_signal_type<std::string>::value);
}
