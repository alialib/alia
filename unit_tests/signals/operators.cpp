#define ALIA_LOWERCASE_MACROS

#include <alia/signals/operators.hpp>

#include <map>

#include <catch.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>
#include <alia/signals/utilities.hpp>

using namespace alia;

template<class Signal>
bool
is_true(Signal const& x)
{
    return signal_is_readable(x) && read_signal(x);
}

template<class Signal>
bool
is_false(Signal const& x)
{
    return signal_is_readable(x) && !read_signal(x);
}

TEST_CASE("basic signal operators", "[signals][operators][operators]")
{
    REQUIRE(is_true(val(2) == val(2)));
    REQUIRE(is_false(val(6) == val(2)));
    REQUIRE(is_true(val(6) != val(2)));
    REQUIRE(is_false(val(2) != val(2)));
    REQUIRE(is_true(val(6) > val(2)));
    REQUIRE(is_false(val(6) < val(2)));
    REQUIRE(is_true(val(6) >= val(2)));
    REQUIRE(is_true(val(2) >= val(2)));
    REQUIRE(is_false(val(2) >= val(6)));
    REQUIRE(is_true(val(2) < val(6)));
    REQUIRE(is_false(val(6) < val(2)));
    REQUIRE(is_true(val(2) <= val(6)));
    REQUIRE(is_true(val(2) <= val(2)));
    REQUIRE(is_false(val(6) <= val(2)));

    REQUIRE(is_true(val(6) + val(2) == val(8)));
    REQUIRE(is_true(val(6) - val(2) == val(4)));
    REQUIRE(is_true(val(6) * val(2) == val(12)));
    REQUIRE(is_true(val(6) / val(2) == val(3)));
    REQUIRE(is_true(val(6) % val(2) == val(0)));
    REQUIRE(is_true((val(6) ^ val(2)) == val(4)));
    REQUIRE(is_true((val(6) & val(2)) == val(2)));
    REQUIRE(is_true((val(6) | val(2)) == val(6)));
    REQUIRE(is_true(val(6) << val(2) == val(24)));
    REQUIRE(is_true(val(6) >> val(2) == val(1)));

    REQUIRE(is_true(val(6) + 2 == val(8)));
    REQUIRE(is_true(6 + val(2) == val(8)));
    REQUIRE(is_true(val(6) + val(2) == 8));

    REQUIRE(is_true(-val(2) == val(-2)));
    REQUIRE(is_false(!(val(2) == val(2))));
}

TEST_CASE("signal &&", "[signals][operators][operators]")
{
    REQUIRE(is_true(val(true) && val(true)));
    REQUIRE(is_false(val(true) && val(false)));
    REQUIRE(is_false(val(false) && val(true)));
    REQUIRE(is_false(val(false) && val(false)));

    // Check that unreadable signals are treated properly.
    REQUIRE(!signal_is_readable(empty<bool>() && empty<bool>()));
    REQUIRE(!signal_is_readable(val(true) && empty<bool>()));
    REQUIRE(!signal_is_readable(empty<bool>() && val(true)));
    REQUIRE(is_false(val(false) && empty<bool>()));
    REQUIRE(is_false(empty<bool>() && val(false)));

    // Check that && short-circuits.
    int access_count = 0;
    auto access_counting_signal = lambda_reader(always_readable, [&]() {
        ++access_count;
        return true;
    });
    REQUIRE(is_false(val(false) && access_counting_signal));
    REQUIRE(access_count == 0);

    // Check that its value ID behaves reasonably.
    REQUIRE(
        (val(true) && val(false)).value_id()
        != (val(true) && val(true)).value_id());

    // Check that it works with plain old values too.
    REQUIRE(is_true(true && val(true)));
    REQUIRE(is_true(val(true) && true));
    REQUIRE(is_false(true && val(false)));
    REQUIRE(is_false(val(false) && true));
}

TEST_CASE("signal ||", "[signals][operators]")
{
    REQUIRE(is_true(val(true) || val(true)));
    REQUIRE(is_true(val(true) || val(false)));
    REQUIRE(is_true(val(false) || val(true)));
    REQUIRE(is_false(val(false) || val(false)));

    // Check that unreadable signals are treated properly.
    REQUIRE(!signal_is_readable(empty<bool>() || empty<bool>()));
    REQUIRE(!signal_is_readable(val(false) || empty<bool>()));
    REQUIRE(!signal_is_readable(empty<bool>() || val(false)));
    REQUIRE(is_true(val(true) || empty<bool>()));
    REQUIRE(is_true(empty<bool>() || val(true)));

    // Check that || short-circuits.
    int access_count = 0;
    auto access_counting_signal = lambda_reader(always_readable, [&]() {
        ++access_count;
        return false;
    });
    REQUIRE(is_true(val(true) || access_counting_signal));
    REQUIRE(access_count == 0);

    // Check that its value ID behaves reasonably.
    REQUIRE(
        (val(false) || val(false)).value_id()
        != (val(true) || val(false)).value_id());

    // Check that it works with plain old values too.
    REQUIRE(is_true(true || val(false)));
    REQUIRE(is_true(val(false) || true));
    REQUIRE(is_false(false || val(false)));
    REQUIRE(is_false(val(false) || false));
}

TEST_CASE("signal select", "[signals][operators]")
{
    bool condition = false;
    auto s = conditional(direct(condition), val(1), val(2));

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

TEST_CASE("select with different directions", "[signals][operators]")
{
    bool condition = false;
    auto s = conditional(direct(condition), empty<int>(), val(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(!signal_is_readable(s));
}

TEST_CASE("select value ID", "[signals][operators]")
{
    // Test that conditional's ID changes when the condition changes, even if
    // both of its input signals are producing the same value ID.

    bool condition = false;
    auto s = conditional(direct(condition), val(2), val(2));

    captured_id original_id = s.value_id();
    condition = true;
    REQUIRE(original_id.get() != s.value_id());
}

TEST_CASE("select with unreadable condition", "[signals][operators]")
{
    int x = 0, y = 1;
    auto s = conditional(empty<bool>(), direct(x), direct(y));
    REQUIRE(!signal_is_readable(s));
    REQUIRE(s.value_id() == no_id);
    REQUIRE(!signal_is_writable(s));
}

TEST_CASE("writable select", "[signals][operators]")
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

TEST_CASE("field signal", "[signals][operators]")
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
TEST_CASE("subscript metafunctions", "[signals][operators]")
{
    REQUIRE((has_at_indexer<std::vector<int>, int>::value));
    REQUIRE((has_at_indexer<std::map<int, int>, int>::value));
    REQUIRE((has_at_indexer<std::vector<bool>, int>::value));
    REQUIRE((!has_at_indexer<my_array, int>::value));
    REQUIRE((!has_at_indexer<my_const_array, int>::value));

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

    REQUIRE((const_subscript_returns_reference<std::vector<int>, int>::value));
    REQUIRE(
        (const_subscript_returns_reference<std::map<int, int>, int>::value));
    REQUIRE(
        (!const_subscript_returns_reference<std::vector<bool>, int>::value));
    REQUIRE((const_subscript_returns_reference<my_array, int>::value));
    REQUIRE((!const_subscript_returns_reference<my_const_array, int>::value));
}

TEST_CASE("vector subscript", "[signals][operators]")
{
    using namespace alia;

    auto c = std::vector<int>{2, 0, 3};
    auto c_signal = direct(c);
    auto s = c_signal[val(1)];

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
    auto t = c_signal[val(0)];
    REQUIRE(t.value_id() != s.value_id());
}

TEST_CASE("read-only subscript", "[signals][operators]")
{
    using namespace alia;

    auto c = std::vector<int>{2, 0, 3};
    auto c_signal = val(c);
    auto s = c_signal[val(1)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 0);
}

TEST_CASE("vector<bool> subscript", "[signals][operators]")
{
    using namespace alia;

    auto c = std::vector<bool>{true, false, false};
    auto c_signal = direct(c);
    auto s = c_signal[val(1)];

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

TEST_CASE("map subscript", "[signals][operators]")
{
    using namespace alia;

    auto c = std::map<int, int>{{2, 1}, {0, 3}};
    auto c_signal = direct(c);
    auto s = c_signal[val(2)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 7);
    REQUIRE((c == std::map<int, int>{{2, 7}, {0, 3}}));
}

TEST_CASE("custom ref subscript", "[signals][operators]")
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
    auto s = c_signal[val(2)];

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

TEST_CASE("custom by-value subscript", "[signals][operators]")
{
    my_const_array c;
    auto c_signal = lambda_reader(
        always_readable,
        [&]() { return c; },
        [&]() {
            return unit_id; // doesn't really matter
        });
    auto s = c_signal[val(2)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 3);
}
