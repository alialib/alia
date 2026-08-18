#include <alia/kernel/signals/operators.hpp>

#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

using namespace alia;

template<class Signal>
bool
is_true(Signal const& x)
{
    return signal_has_value(x) && read_signal(x);
}

template<class Signal>
bool
is_false(Signal const& x)
{
    return signal_has_value(x) && !read_signal(x);
}

TEST_CASE("basic signal operators")
{
    CHECK(is_true(value(2) == value(2)));
    CHECK(is_false(value(6) == value(2)));
    CHECK(is_true(value(6) != value(2)));
    CHECK(is_false(value(2) != value(2)));
    CHECK(is_true(value(6) > value(2)));
    CHECK(is_false(value(6) < value(2)));
    CHECK(is_true(value(6) >= value(2)));
    CHECK(is_true(value(2) >= value(2)));
    CHECK(is_false(value(2) >= value(6)));
    CHECK(is_true(value(2) < value(6)));
    CHECK(is_false(value(6) < value(2)));
    CHECK(is_true(value(2) <= value(6)));
    CHECK(is_true(value(2) <= value(2)));
    CHECK(is_false(value(6) <= value(2)));

    CHECK(is_true(value(6) + value(2) == value(8)));
    CHECK(is_true(value(6) - value(2) == value(4)));
    CHECK(is_true(value(6) * value(2) == value(12)));
    CHECK(is_true(value(6) / value(2) == value(3)));
    CHECK(is_true(value(6) % value(2) == value(0)));
    CHECK(is_true((value(6) ^ value(2)) == value(4)));
    CHECK(is_true((value(6) & value(2)) == value(2)));
    CHECK(is_true((value(6) | value(2)) == value(6)));
    CHECK(is_true(value(6) << value(2) == value(24)));
    CHECK(is_true(value(6) >> value(2) == value(1)));

    CHECK(is_true(value(6) + 2 == value(8)));
    CHECK(is_true(6 + value(2) == value(8)));
    CHECK(is_true(value(6) + value(2) == 8));

    CHECK(is_true(-value(2) == value(-2)));
    CHECK(is_false(!(value(2) == value(2))));
}
