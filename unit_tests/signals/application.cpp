#define ALIA_LOWERCASE_MACROS

#include <alia/signals/application.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>

using namespace alia;

TEST_CASE("lazy_apply", "[signals]")
{
    auto s1 = lazy_apply([](int i) { return 2 * i; }, value(1));

    typedef decltype(s1) signal_t1;
    REQUIRE(signal_can_read<signal_t1>::value);
    REQUIRE(!signal_can_write<signal_t1>::value);

    REQUIRE(signal_is_readable(s1));
    REQUIRE(read_signal(s1) == 2);

    auto s2
        = lazy_apply([](int i, int j) { return i + j; }, value(1), value(6));

    typedef decltype(s2) signal_t2;
    REQUIRE(signal_can_read<signal_t2>::value);
    REQUIRE(!signal_can_write<signal_t2>::value);

    REQUIRE(signal_is_readable(s2));
    REQUIRE(read_signal(s2) == 7);
    REQUIRE(s1.value_id() != s2.value_id());

    // Create some similar signals to make sure that they produce different
    // value IDs.
    auto s3
        = lazy_apply([](int i, int j) { return i + j; }, value(2), value(6));
    auto s4
        = lazy_apply([](int i, int j) { return i + j; }, value(1), value(0));
    REQUIRE(s2.value_id() != s3.value_id());
    REQUIRE(s2.value_id() != s4.value_id());
    REQUIRE(s3.value_id() != s4.value_id());
}

TEST_CASE("alia_method", "[signals]")
{
    auto v = value("test text");
    REQUIRE(read_signal(lazy_apply(alia_method(length), v)) == 9);
}
