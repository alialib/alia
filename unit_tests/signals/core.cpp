#include <alia/signals/core.hpp>

#include <testing.hpp>

#include <alia/signals/basic.hpp>

using namespace alia;

TEST_CASE("signal_direction_is_compatible", "[signals][core]")
{
#define TEST_COMPATIBILITY(Expected, Actual, result)                           \
    REQUIRE(                                                                   \
        (signal_direction_is_compatible<Expected, Actual>::value) == (result))

    TEST_COMPATIBILITY(read_only_signal, read_only_signal, true);
    TEST_COMPATIBILITY(read_only_signal, write_only_signal, false);
    TEST_COMPATIBILITY(read_only_signal, duplex_signal, true);
    TEST_COMPATIBILITY(write_only_signal, read_only_signal, false);
    TEST_COMPATIBILITY(write_only_signal, write_only_signal, true);
    TEST_COMPATIBILITY(write_only_signal, duplex_signal, true);
    TEST_COMPATIBILITY(duplex_signal, read_only_signal, false);
    TEST_COMPATIBILITY(duplex_signal, write_only_signal, false);
    TEST_COMPATIBILITY(duplex_signal, duplex_signal, true);
}

TEST_CASE("signal_direction_intersection", "[signals][core]")
{
#define TEST_INTERSECTION(A, B, Result)                                        \
    REQUIRE((std::is_same<signal_direction_intersection<A, B>::type, Result>:: \
                 value))

    TEST_INTERSECTION(read_only_signal, read_only_signal, read_only_signal);
    TEST_INTERSECTION(read_only_signal, duplex_signal, read_only_signal);
    TEST_INTERSECTION(write_only_signal, write_only_signal, write_only_signal);
    TEST_INTERSECTION(write_only_signal, duplex_signal, write_only_signal);
    TEST_INTERSECTION(duplex_signal, read_only_signal, read_only_signal);
    TEST_INTERSECTION(duplex_signal, write_only_signal, write_only_signal);
    TEST_INTERSECTION(duplex_signal, duplex_signal, duplex_signal);
}

TEST_CASE("signal_direction_union", "[signals][core]")
{
#define TEST_UNION(A, B, Result)                                               \
    REQUIRE((std::is_same<signal_direction_union<A, B>::type, Result>::value))

    TEST_UNION(read_only_signal, read_only_signal, read_only_signal);
    typedef signal_directionality<signal_readable, signal_writable>
        read_write_signal;
    TEST_UNION(read_only_signal, write_only_signal, read_write_signal);
    TEST_UNION(read_only_signal, duplex_signal, duplex_signal);
    TEST_UNION(write_only_signal, write_only_signal, write_only_signal);
    TEST_UNION(write_only_signal, read_only_signal, read_write_signal);
    TEST_UNION(write_only_signal, duplex_signal, duplex_signal);
    TEST_UNION(duplex_signal, read_only_signal, duplex_signal);
    TEST_UNION(duplex_signal, write_only_signal, duplex_signal);
    TEST_UNION(duplex_signal, duplex_signal, duplex_signal);
}

TEST_CASE("is_signal_type", "[signals][core]")
{
    REQUIRE(is_signal_type<readable<int>>::value);
    REQUIRE(is_signal_type<writable<int>>::value);
    REQUIRE(is_signal_type<duplex<int>>::value);
    REQUIRE(!is_signal_type<int>::value);
    REQUIRE(!is_signal_type<std::string>::value);
}

TEST_CASE("signal_is_readable", "[signals][core]")
{
    REQUIRE(signal_is_readable<readable<int>>::value);
    REQUIRE(!signal_is_readable<writable<int>>::value);
    REQUIRE(signal_is_readable<duplex<int>>::value);
}

TEST_CASE("is_readable_signal_type", "[signals][core]")
{
    REQUIRE(is_readable_signal_type<readable<int>>::value);
    REQUIRE(!is_readable_signal_type<writable<int>>::value);
    REQUIRE(is_readable_signal_type<duplex<int>>::value);
    REQUIRE(!is_readable_signal_type<int>::value);
    REQUIRE(!is_readable_signal_type<std::string>::value);
}

TEST_CASE("signal_is_writable", "[signals][core]")
{
    REQUIRE(!signal_is_writable<readable<int>>::value);
    REQUIRE(signal_is_writable<writable<int>>::value);
    REQUIRE(signal_is_writable<duplex<int>>::value);
}

TEST_CASE("is_writable_signal_type", "[signals][core]")
{
    REQUIRE(!is_writable_signal_type<readable<int>>::value);
    REQUIRE(is_writable_signal_type<writable<int>>::value);
    REQUIRE(is_writable_signal_type<duplex<int>>::value);
    REQUIRE(!is_writable_signal_type<int>::value);
    REQUIRE(!is_writable_signal_type<std::string>::value);
}

TEST_CASE("is_duplex_signal_type", "[signals][core]")
{
    REQUIRE(!is_duplex_signal_type<readable<int>>::value);
    REQUIRE(!is_duplex_signal_type<writable<int>>::value);
    REQUIRE(is_duplex_signal_type<duplex<int>>::value);
    REQUIRE(!is_duplex_signal_type<int>::value);
    REQUIRE(!is_duplex_signal_type<std::string>::value);
}

TEST_CASE("signal_ref", "[signals][core]")
{
    int x = 1;
    auto y = direct(x);
    signal_ref<int, duplex_signal> s = y;

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(s.value_id() == y.value_id());
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 0);
    REQUIRE(x == 0);
    REQUIRE(read_signal(s) == 0);
}

static void f_readable(alia::readable<int>)
{
}

static void f_writable(alia::writable<int>)
{
}

static void f_duplex(alia::duplex<int>)
{
}

TEST_CASE("signal parameter passing", "[signals][core]")
{
    auto read_only = value(0);
    int x = 0;
    auto duplex = direct(x);

    f_readable(read_only);
    f_readable(duplex);
    f_writable(duplex);
    f_duplex(duplex);
}
