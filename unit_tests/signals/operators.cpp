#include <alia/signals/operators.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>
#include <alia/signals/utilities.hpp>

TEST_CASE("signal operators", "[signals]")
{
    using namespace alia;

    REQUIRE(is_true(constant(2) == constant(2)));
    REQUIRE(is_false(constant(6) == constant(2)));
    REQUIRE(is_true(constant(6) != constant(2)));
    REQUIRE(is_false(constant(2) != constant(2)));
    REQUIRE(is_true(constant(6) > constant(2)));
    REQUIRE(is_false(constant(6) < constant(2)));
    REQUIRE(is_true(constant(6) >= constant(2)));
    REQUIRE(is_true(constant(2) >= constant(2)));
    REQUIRE(is_false(constant(2) >= constant(6)));
    REQUIRE(is_true(constant(2) < constant(6)));
    REQUIRE(is_false(constant(6) < constant(2)));
    REQUIRE(is_true(constant(2) <= constant(6)));
    REQUIRE(is_true(constant(2) <= constant(2)));
    REQUIRE(is_false(constant(6) <= constant(2)));

    REQUIRE(is_true(constant(6) + constant(2) == constant(8)));
    REQUIRE(is_true(constant(6) - constant(2) == constant(4)));
    REQUIRE(is_true(constant(6) * constant(2) == constant(12)));
    REQUIRE(is_true(constant(6) / constant(2) == constant(3)));
    REQUIRE(is_true(constant(6) % constant(2) == constant(0)));
    REQUIRE(is_true((constant(6) ^ constant(2)) == constant(4)));
    REQUIRE(is_true((constant(6) & constant(2)) == constant(2)));
    REQUIRE(is_true((constant(6) | constant(2)) == constant(6)));
    REQUIRE(is_true(constant(6) << constant(2) == constant(24)));
    REQUIRE(is_true(constant(6) >> constant(2) == constant(1)));

    REQUIRE(is_true(-constant(2) == constant(-2)));
    REQUIRE(is_false(!(constant(2) == constant(2))));
}

TEST_CASE("select_signal", "[signals]")
{
    using namespace alia;

    bool condition = false;
    auto s = select_signal(direct(&condition), constant(1), constant(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
}

TEST_CASE("select_signal with different directions", "[signals]")
{
    using namespace alia;

    bool condition = false;
    auto s = select_signal(direct(&condition), empty<int>(), constant(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(!signal_is_readable(s));
}

TEST_CASE("writable select_signal", "[signals]")
{
    using namespace alia;

    bool condition = false;
    int x = 1;
    int y = 2;
    auto s = select_signal(direct(&condition), direct(&x), direct(&y));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(read_signal(s) == 1);
    write_signal(s, 4);
    REQUIRE(x == 4);
    REQUIRE(y == 2);
    REQUIRE(read_signal(s) == 4);
    condition = false;
    write_signal(s, 3);
    REQUIRE(x == 4);
    REQUIRE(y == 3);
    REQUIRE(read_signal(s) == 3);
}

TEST_CASE("field signal", "[signals]")
{
    using namespace alia;

    struct foo
    {
        int x;
        double y;
    };
    foo f = {2, 1.5};
    auto f_signal = lambda_inout(
        always_readable,
        [&]() { return f; },
        always_writable,
        [&](foo const& v) { f = v; },
        [&]() { return combine_ids(make_id(f.x), make_id(f.y)); });

    auto x_signal = f_signal->*&foo::x;

    typedef decltype(x_signal) x_signal_t;
    REQUIRE((std::is_same<x_signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<x_signal_t>::value);
    REQUIRE(signal_can_write<x_signal_t>::value);

    REQUIRE(signal_is_readable(x_signal));
    REQUIRE(read_signal(x_signal) == 2);
    REQUIRE(signal_is_writable(x_signal));
    write_signal(x_signal, 1);
    REQUIRE(f.x == 1);

    auto y_signal = alia_field(f_signal, y);

    typedef decltype(y_signal) y_signal_t;
    REQUIRE((std::is_same<y_signal_t::value_type, double>::value));
    REQUIRE(signal_can_read<y_signal_t>::value);
    REQUIRE(signal_can_write<y_signal_t>::value);

    REQUIRE(y_signal.value_id() != x_signal.value_id());
    REQUIRE(signal_is_readable(y_signal));
    REQUIRE(read_signal(y_signal) == 1.5);
    REQUIRE(signal_is_writable(y_signal));
    owned_id captured_y_id;
    captured_y_id.store(y_signal.value_id());
    write_signal(y_signal, 0.5);
    REQUIRE(y_signal.value_id() != captured_y_id.get());
    REQUIRE(f.y == 0.5);
}
