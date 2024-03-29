#define ALIA_LOWERCASE_MACROS

#include <alia/core/signals/operators.hpp>

#include <map>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <alia/core/actions/operators.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/lambdas.hpp>
#include <alia/core/signals/state.hpp>
#include <alia/core/signals/utilities.hpp>

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

TEST_CASE("basic signal operators", "[signals][operators]")
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

    REQUIRE(is_true(value(6) + 2 == value(8)));
    REQUIRE(is_true(6 + value(2) == value(8)));
    REQUIRE(is_true(value(6) + value(2) == 8));

    REQUIRE(is_true(-value(2) == value(-2)));
    REQUIRE(is_false(!(value(2) == value(2))));
}

TEST_CASE("unary operator *", "[signals][operators]")
{
    auto x = value(std::optional<int>(2));
    REQUIRE(is_true(*x == value(2)));
    REQUIRE(is_false(*x + 1 == value(2)));
}

TEST_CASE("signal &&", "[signals][operators]")
{
    REQUIRE(is_true(value(true) && value(true)));
    REQUIRE(is_false(value(true) && value(false)));
    REQUIRE(is_false(value(false) && value(true)));
    REQUIRE(is_false(value(false) && value(false)));

    // Check that empty signals are treated properly.
    REQUIRE(!signal_has_value(empty<bool>() && empty<bool>()));
    REQUIRE(!signal_has_value(value(true) && empty<bool>()));
    REQUIRE(!signal_has_value(empty<bool>() && value(true)));
    REQUIRE(is_false(value(false) && empty<bool>()));
    REQUIRE(is_false(empty<bool>() && value(false)));

    // Check that && short-circuits.
    int access_count = 0;
    auto access_counting_signal = lambda_reader(always_has_value, [&]() {
        ++access_count;
        return true;
    });
    REQUIRE(is_false(value(false) && access_counting_signal));
    REQUIRE(access_count == 0);

    // Check that its value ID behaves reasonably.
    REQUIRE(
        (value(true) && value(false)).value_id()
        != (value(true) && value(true)).value_id());

    // Check that it works with plain old values too.
    REQUIRE(is_true(true && value(true)));
    REQUIRE(is_true(value(true) && true));
    REQUIRE(is_false(true && value(false)));
    REQUIRE(is_false(value(false) && true));
}

TEST_CASE("signal ||", "[signals][operators]")
{
    REQUIRE(is_true(value(true) || value(true)));
    REQUIRE(is_true(value(true) || value(false)));
    REQUIRE(is_true(value(false) || value(true)));
    REQUIRE(is_false(value(false) || value(false)));

    // Check that empty signals are treated properly.
    REQUIRE(!signal_has_value(empty<bool>() || empty<bool>()));
    REQUIRE(!signal_has_value(value(false) || empty<bool>()));
    REQUIRE(!signal_has_value(empty<bool>() || value(false)));
    REQUIRE(is_true(value(true) || empty<bool>()));
    REQUIRE(is_true(empty<bool>() || value(true)));

    // Check that || short-circuits.
    int access_count = 0;
    auto access_counting_signal = lambda_reader(always_has_value, [&]() {
        ++access_count;
        return false;
    });
    REQUIRE(is_true(value(true) || access_counting_signal));
    REQUIRE(access_count == 0);

    // Check that its value ID behaves reasonably.
    REQUIRE(
        (value(false) || value(false)).value_id()
        != (value(true) || value(false)).value_id());

    // Check that it works with plain old values too.
    REQUIRE(is_true(true || value(false)));
    REQUIRE(is_true(value(false) || true));
    REQUIRE(is_false(false || value(false)));
    REQUIRE(is_false(value(false) || false));
}

TEST_CASE("signal select", "[signals][operators]")
{
    bool condition = false;
    auto s = conditional(direct(condition), value(1), value(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 2);
    captured_id original_id = s.value_id();
    condition = true;
    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(*original_id != s.value_id());
}

TEST_CASE("non-boolean signal select", "[signals][operators]")
{
    auto x = value(std::optional<int>(2));
    REQUIRE(is_true(conditional(x, *x, value(0)) == value(2)));
    auto y = value(std::optional<int>());
    REQUIRE(is_true(conditional(y, *y, value(0)) == value(0)));
}

TEST_CASE("select with different capabilities", "[signals][operators]")
{
    bool condition = false;
    auto s = conditional(direct(condition), empty<int>(), value(2));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 2);
    condition = true;
    REQUIRE(!signal_has_value(s));
}

TEST_CASE("select value ID", "[signals][operators]")
{
    // Test that conditional's ID changes when the condition changes, even if
    // both of its input signals are producing the same value ID.

    bool condition = false;
    auto s = conditional(direct(condition), value(2), value(2));

    captured_id original_id = s.value_id();
    condition = true;
    REQUIRE(*original_id != s.value_id());
}

TEST_CASE("select with empty condition", "[signals][operators]")
{
    int x = 0, y = 1;
    auto s = conditional(empty<bool>(), direct(x), direct(y));
    REQUIRE(!signal_has_value(s));
    REQUIRE(s.value_id() == null_id);
    REQUIRE(!signal_ready_to_write(s));
}

TEST_CASE("writable select", "[signals][operators]")
{
    bool condition = false;
    int x = 1;
    int y = 2;
    auto s = conditional(direct(condition), direct(x), direct(y));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
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
        std::string y;
    };
    foo f = {2, "1.5"};
    auto f_signal = lambda_duplex(
        always_has_value,
        [&]() { return f; },
        always_ready,
        [&](foo const& v) { f = v; },
        [&]() { return combine_ids(make_id(f.x), make_id(f.y)); });
    auto x_signal = f_signal->*&foo::x;

    typedef decltype(x_signal) x_signal_t;
    REQUIRE((std::is_same<x_signal_t::value_type, int>::value));
    REQUIRE(signal_is_readable<x_signal_t>::value);
    REQUIRE(signal_is_writable<x_signal_t>::value);

    REQUIRE(signal_has_value(x_signal));
    REQUIRE(read_signal(x_signal) == 2);
    REQUIRE(signal_ready_to_write(x_signal));
    write_signal(x_signal, 1);
    REQUIRE(f.x == 1);

    auto y_signal = alia_field(f_signal, y);

    typedef decltype(y_signal) y_signal_t;
    REQUIRE((std::is_same<y_signal_t::value_type, std::string>::value));
    REQUIRE(signal_is_readable<y_signal_t>::value);
    REQUIRE(signal_is_writable<y_signal_t>::value);

    REQUIRE(y_signal.value_id() != x_signal.value_id());
    REQUIRE(signal_has_value(y_signal));
    REQUIRE(read_signal(y_signal) == "1.5");
    REQUIRE(signal_ready_to_write(y_signal));
    captured_id original_y_id = y_signal.value_id();
    write_signal(y_signal, "0.5");
    REQUIRE(y_signal.value_id() != *original_y_id);
    REQUIRE(f.y == "0.5");
}

struct my_array
{
    int x[3] = {1, 2, 3};
    int&
    operator[](int i)
    {
        return x[i];
    }
    int const&
    operator[](int i) const
    {
        return x[i];
    }
};

struct my_const_array
{
    int x[3] = {1, 2, 3};
    int
    operator[](int i) const
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
    REQUIRE((
        std::is_same<subscript_result_type<my_array, int>::type, int>::value));
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
    auto s = c_signal[value(1)];

#ifdef NDEBUG
    BENCHMARK("subscript signal creation")
    {
        return c_signal[value(1)];
    };
#endif

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 0);
    REQUIRE(signal_ready_to_write(s));
    captured_id original_id = s.value_id();
    write_signal(s, 1);
    REQUIRE(c == std::vector<int>({2, 1, 3}));

    // Check that changes in the container and the index both cause changes
    // in the value ID.
    REQUIRE(!original_id.matches(s.value_id()));
    auto t = c_signal[value(0)];
    REQUIRE(t.value_id() != s.value_id());
}

TEST_CASE("read-only subscript", "[signals][operators]")
{
    using namespace alia;

    auto c = std::vector<int>{2, 0, 3};
    auto c_signal = value(c);
    auto s = c_signal[value(1)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 0);
}

TEST_CASE("subscript with raw index", "[signals][operators]")
{
    using namespace alia;

    auto c = std::vector<int>{2, 0, 3};
    auto c_signal = value(c);
    auto s = c_signal[1];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 0);
}

TEST_CASE("vector<bool> subscript", "[signals][operators]")
{
    using namespace alia;

    auto c = std::vector<bool>{true, false, false};
    auto c_signal = direct(c);
    auto s = c_signal[value(1)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, bool>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == false);
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, true);
    REQUIRE(c == std::vector<bool>({true, true, false}));
}

TEST_CASE("map subscript", "[signals][operators]")
{
    using namespace alia;

    auto c = std::map<int, int>{{2, 1}, {0, 3}};
    auto c_signal = direct(c);
    auto s = c_signal[value(2)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 7);
    REQUIRE((c == std::map<int, int>{{2, 7}, {0, 3}}));
}

TEST_CASE("custom ref subscript", "[signals][operators]")
{
    my_array c;
    auto c_signal = lambda_duplex(
        always_has_value,
        [&]() { return c; },
        always_ready,
        [&](my_array const& v) { c = v; },
        [&]() {
            return unit_id; // doesn't really matter
        });
    auto s = c_signal[value(2)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 3);
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 4);
    REQUIRE(c[2] == 4);
}

TEST_CASE("custom by-value subscript", "[signals][operators]")
{
    my_const_array c;
    auto c_signal = lambda_reader(
        always_has_value,
        [&]() { return c; },
        [&]() {
            return unit_id; // doesn't really matter
        });
    auto s = c_signal[value(2)];

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, int>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 3);
}

TEST_CASE("empty subscript", "[signals][operators]")
{
    using namespace alia;

    auto s = empty<std::map<int, int>>()[value(2)];

    REQUIRE(!signal_has_value(s));
    REQUIRE(s.value_id() == null_id);
}

TEST_CASE("state subscript", "[signals][operators]")
{
    using namespace alia;

    state_storage<std::map<int, int>> state;
    state.set(std::map<int, int>());
    auto state_signal = make_state_signal(state);

    auto s = state_signal[value(2)];

    perform_action(s <<= value(3));
    REQUIRE(state.nonconst_ref()[2] == 3);
}
