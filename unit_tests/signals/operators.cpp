#define ALIA_LOWERCASE_MACROS

#include <alia/signals/operators.hpp>

#include <map>

#include <catch2/catch.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>
#include <alia/signals/utilities.hpp>

using namespace alia;

TEST_CASE("basic signal operators", "[signals]")
{
    REQUIRE(is_true(value(2) == value(2)));
    REQUIRE(is_false(value(6) == value(2)));
    REQUIRE(is_true(value(6) != value(2)));
    REQUIRE(is_false(value(2) != value(2)));
    REQUIRE(is_true(value(6) > value(2)));
    REQUIRE(is_false(value(6) < value(2)));
    REQUIRE(is_true(value(6) >= value(2)));
    REQUIRE(is_true(value(2) >= value(2)));
    REQUIRE(is_false(value(2) >= value(6)));
    REQUIRE(is_true(value(2) < value(6)));
    REQUIRE(is_false(value(6) < value(2)));
    REQUIRE(is_true(value(2) <= value(6)));
    REQUIRE(is_true(value(2) <= value(2)));
    REQUIRE(is_false(value(6) <= value(2)));

    REQUIRE(is_true(value(6) + value(2) == value(8)));
    REQUIRE(is_true(value(6) - value(2) == value(4)));
    REQUIRE(is_true(value(6) * value(2) == value(12)));
    REQUIRE(is_true(value(6) / value(2) == value(3)));
    REQUIRE(is_true(value(6) % value(2) == value(0)));
    REQUIRE(is_true((value(6) ^ value(2)) == value(4)));
    REQUIRE(is_true((value(6) & value(2)) == value(2)));
    REQUIRE(is_true((value(6) | value(2)) == value(6)));
    REQUIRE(is_true(value(6) << value(2) == value(24)));
    REQUIRE(is_true(value(6) >> value(2) == value(1)));

    REQUIRE(is_true(-value(2) == value(-2)));
    REQUIRE(is_false(!(value(2) == value(2))));
}

TEST_CASE("signal &&", "[signals]")
{
    REQUIRE(is_true(value(true) && value(true)));
    REQUIRE(is_false(value(true) && value(false)));
    REQUIRE(is_false(value(false) && value(true)));
    REQUIRE(is_false(value(false) && value(false)));

    // Check that unreadable signals are treated properly.
    REQUIRE(!signal_is_readable(empty<bool>() && empty<bool>()));
    REQUIRE(!signal_is_readable(value(true) && empty<bool>()));
    REQUIRE(!signal_is_readable(empty<bool>() && value(true)));
    REQUIRE(is_false(value(false) && empty<bool>()));
    REQUIRE(is_false(empty<bool>() && value(false)));

    // Check that && short-circuits.
    int access_count = 0;
    auto access_counting_signal = lambda_input(always_readable, [&]() {
        ++access_count;
        return true;
    });
    REQUIRE(is_false(value(false) && access_counting_signal));
    REQUIRE(access_count == 0);

    // Check that its value ID behaves reasonably.
    REQUIRE(
        (value(true) && value(false)).value_id()
        != (value(true) && value(true)).value_id());
}

TEST_CASE("signal ||", "[signals]")
{
    REQUIRE(is_true(value(true) || value(true)));
    REQUIRE(is_true(value(true) || value(false)));
    REQUIRE(is_true(value(false) || value(true)));
    REQUIRE(is_false(value(false) || value(false)));

    // Check that unreadable signals are treated properly.
    REQUIRE(!signal_is_readable(empty<bool>() || empty<bool>()));
    REQUIRE(!signal_is_readable(value(false) || empty<bool>()));
    REQUIRE(!signal_is_readable(empty<bool>() || value(false)));
    REQUIRE(is_true(value(true) || empty<bool>()));
    REQUIRE(is_true(empty<bool>() || value(true)));

    // Check that || short-circuits.
    int access_count = 0;
    auto access_counting_signal = lambda_input(always_readable, [&]() {
        ++access_count;
        return false;
    });
    REQUIRE(is_true(value(true) || access_counting_signal));
    REQUIRE(access_count == 0);

    // Check that its value ID behaves reasonably.
    REQUIRE(
        (value(false) || value(false)).value_id()
        != (value(true) || value(false)).value_id());
}

TEST_CASE("signal select", "[signals]")
{
    bool condition = false;
    auto s = conditional(direct(condition), value(1), value(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    captured_id original_id = s.value_id();
    condition = true;
    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(original_id.get() != s.value_id());
}

TEST_CASE("select with different directions", "[signals]")
{
    bool condition = false;
    auto s = conditional(direct(condition), empty<int>(), value(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(!signal_is_readable(s));
}

TEST_CASE("select value ID", "[signals]")
{
    // Test that conditional's ID changes when the condition changes, even if
    // both of its input signals are producing the same value ID.

    bool condition = false;
    auto s = conditional(direct(condition), value(2), value(2));

    captured_id original_id = s.value_id();
    condition = true;
    REQUIRE(original_id.get() != s.value_id());
}

TEST_CASE("select with unreadable condition", "[signals]")
{
    int x = 0, y = 1;
    auto s = conditional(empty<bool>(), direct(x), direct(y));
    REQUIRE(!signal_is_readable(s));
    REQUIRE(s.value_id() == no_id);
    REQUIRE(!signal_is_writable(s));
}

TEST_CASE("writable select", "[signals]")
{
    bool condition = false;
    int x = 1;
    int y = 2;
    auto s = conditional(direct(condition), direct(x), direct(y));

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
    struct foo
    {
        int x;
        double y;
    };
    foo f = {2, 1.5};
    auto f_signal = lambda_bidirectional(
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
    captured_id original_y_id = y_signal.value_id();
    write_signal(y_signal, 0.5);
    REQUIRE(y_signal.value_id() != original_y_id.get());
    REQUIRE(f.y == 0.5);
}

struct my_array
{
    int x[3] = {1, 2, 3};
    int& operator[](int i)
    {
        return x[i];
    }
    int const& operator[](int i) const
    {
        return x[i];
    }
};

struct my_const_array
{
    int x[3] = {1, 2, 3};
    int operator[](int i) const
    {
        return x[i];
    }
};

// Test some of the helper metafunctions for subscript signals.
TEST_CASE("subscript metafunctions", "[signals]")
{
    REQUIRE(has_at_indexer<std::vector<int>, int>::value);
    REQUIRE(has_at_indexer<std::map<int, int>, int>::value);
    REQUIRE(has_at_indexer<std::vector<bool>, int>::value);
    REQUIRE(!has_at_indexer<my_array, int>::value);
    REQUIRE(!has_at_indexer<my_const_array, int>::value);

    REQUIRE((std::is_same<
             subscript_result_type<std::vector<float>, int>::type,
             float>::value));
    REQUIRE((std::is_same<
             subscript_result_type<std::map<int, float>, int>::type,
             float>::value));
    REQUIRE((std::is_same<
             subscript_result_type<std::vector<bool>, int>::type,
             bool>::value));
    REQUIRE(
        (std::is_same<subscript_result_type<my_array, int>::type, int>::value));
    REQUIRE(
        (std::is_same<subscript_result_type<my_const_array, int>::type, int>::
             value));

    REQUIRE(const_subscript_returns_reference<std::vector<int>, int>::value);
    REQUIRE(const_subscript_returns_reference<std::map<int, int>, int>::value);
    REQUIRE(!const_subscript_returns_reference<std::vector<bool>, int>::value);
    REQUIRE(const_subscript_returns_reference<my_array, int>::value);
    REQUIRE(!const_subscript_returns_reference<my_const_array, int>::value);
}

TEST_CASE("vector subscript", "[signals]")
{
    using namespace alia;

    auto c = std::vector<int>{2, 0, 3};
    auto c_signal = lambda_bidirectional(
        always_readable,
        [&]() { return c; },
        always_writable,
        [&](std::vector<int> const& v) { c = v; },
        [&]() {
            return combine_ids(make_id(c[0]), make_id(c[1]), make_id(c[2]));
        });
    auto s = c_signal[value(1)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 0);
    REQUIRE(signal_is_writable(s));
    captured_id original_id = s.value_id();
    write_signal(s, 1);
    REQUIRE(c == std::vector<int>({2, 1, 3}));

    // Check that changes in the container and the index both cause changes
    // in the value ID.
    REQUIRE(!original_id.matches(s.value_id()));
    auto t = c_signal[value(0)];
    REQUIRE(t.value_id() != s.value_id());
}

TEST_CASE("read-only subscript", "[signals]")
{
    using namespace alia;

    auto c = std::vector<int>{2, 0, 3};
    auto c_signal = lambda_input(
        always_readable,
        [&]() { return c; },
        [&]() {
            return combine_ids(make_id(c[0]), make_id(c[1]), make_id(c[2]));
        });
    auto s = c_signal[value(1)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 0);
}

TEST_CASE("vector<bool> subscript", "[signals]")
{
    using namespace alia;

    auto c = std::vector<bool>{true, false, false};
    auto c_signal = lambda_bidirectional(
        always_readable,
        [&]() { return c; },
        always_writable,
        [&](std::vector<bool> const& v) { c = v; },
        [&]() {
            return unit_id; // doesn't really matter
        });
    auto s = c_signal[value(1)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, bool>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == false);
    REQUIRE(signal_is_writable(s));
    write_signal(s, true);
    REQUIRE(c == std::vector<bool>({true, true, false}));
}

TEST_CASE("map subscript", "[signals]")
{
    using namespace alia;

    auto c = std::map<int, int>{{2, 1}, {0, 3}};
    auto c_signal = lambda_bidirectional(
        always_readable,
        [&]() { return c; },
        always_writable,
        [&](std::map<int, int> const& v) { c = v; },
        [&]() {
            return unit_id; // doesn't really matter
        });
    auto s = c_signal[value(2)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 7);
    REQUIRE(c == std::map<int, int>{{2, 7}, {0, 3}});
}

TEST_CASE("custom ref subscript", "[signals]")
{
    my_array c;
    auto c_signal = lambda_bidirectional(
        always_readable,
        [&]() { return c; },
        always_writable,
        [&](my_array const& v) { c = v; },
        [&]() {
            return unit_id; // doesn't really matter
        });
    auto s = c_signal[value(2)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 3);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 4);
    REQUIRE(c[2] == 4);
}

TEST_CASE("custom by-value subscript", "[signals]")
{
    my_const_array c;
    auto c_signal = lambda_input(
        always_readable,
        [&]() { return c; },
        [&]() {
            return unit_id; // doesn't really matter
        });
    auto s = c_signal[value(2)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 3);
}
