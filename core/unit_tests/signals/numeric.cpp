#include <alia/core/signals/numeric.hpp>

#include <testing.hpp>

#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/utilities.hpp>

using namespace alia;

TEST_CASE("offset signal", "[signals][numeric]")
{
    double x = 1;
    auto s = offset(direct(x), value(0.5));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1.5);
    captured_id id;
    id.capture(s.value_id());
    REQUIRE(id.matches(scale(direct(x), value(0.5)).value_id()));

    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 4);
    REQUIRE(x == 3.5);
    REQUIRE(!id.matches(offset(direct(x), value(0.5)).value_id()));
}

TEST_CASE("scaled signal", "[signals][numeric]")
{
    double x = 1;
    auto s = scale(direct(x), value(0.5));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 0.5);
    captured_id id;
    id.capture(s.value_id());
    REQUIRE(id.matches(scale(direct(x), value(0.5)).value_id()));

    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 2);
    REQUIRE(x == 4);
    REQUIRE(!id.matches(scale(direct(x), value(0.5)).value_id()));
}

TEST_CASE("round_signal_writes", "[signals][numeric]")
{
    double x = 1;
    auto s = round_signal_writes(direct(x), value(0.5));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 0.4);
    REQUIRE(x == 0.5);
}
