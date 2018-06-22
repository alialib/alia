#include <alia/signals/application.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>

TEST_CASE("lazy_apply", "[signals]")
{
    using namespace alia;

    auto s1 = lazy_apply([](int i) { return 2 * i; }, constant(1));

    typedef decltype(s1) signal_t1;
    REQUIRE(signal_can_read<signal_t1>::value);
    REQUIRE(!signal_can_write<signal_t1>::value);

    REQUIRE(signal_is_readable(s1));
    REQUIRE(read_signal(s1) == 2);

    auto s2 = lazy_apply(
        [](int i, int j) { return i + j; }, constant(1), constant(6));

    typedef decltype(s2) signal_t2;
    REQUIRE(signal_can_read<signal_t2>::value);
    REQUIRE(!signal_can_write<signal_t2>::value);

    REQUIRE(signal_is_readable(s2));
    REQUIRE(read_signal(s2) == 7);
}
