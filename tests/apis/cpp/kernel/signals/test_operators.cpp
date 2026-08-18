#include <alia/kernel/signals/operators.hpp>

#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/state.hpp>

#include <doctest/doctest.h>

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

using namespace alia;
using namespace alia::operators;

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

namespace {

struct counting_bool
    : signal<counting_bool, bool, view_caps<signal_readable>>
{
    counting_bool(int* count, bool value) : count_(count), value_(value)
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    bool const&
    read() const override
    {
        ++*count_;
        return value_;
    }
    id_view
    value_id() const override
    {
        return make_id(value_);
    }

 private:
    int* count_;
    bool value_;
};

} // namespace

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

TEST_CASE("signal &&")
{
    CHECK(is_true(value(true) && value(true)));
    CHECK(is_false(value(true) && value(false)));
    CHECK(is_false(value(false) && value(true)));
    CHECK(is_false(value(false) && value(false)));

    CHECK_FALSE(signal_has_value(empty<bool>() && empty<bool>()));
    CHECK_FALSE(signal_has_value(value(true) && empty<bool>()));
    CHECK_FALSE(signal_has_value(empty<bool>() && value(true)));
    CHECK(is_false(value(false) && empty<bool>()));
    CHECK(is_false(empty<bool>() && value(false)));

    int access_count = 0;
    CHECK(is_false(value(false) && counting_bool(&access_count, true)));
    CHECK(access_count == 0);

    CHECK(
        ((value(true) && value(false)).value_id()
         != (value(true) && value(true)).value_id()));

    CHECK(is_true(true && value(true)));
    CHECK(is_true(value(true) && true));
    CHECK(is_false(true && value(false)));
    CHECK(is_false(value(false) && true));
}

TEST_CASE("signal ||")
{
    CHECK(is_true(value(true) || value(true)));
    CHECK(is_true(value(true) || value(false)));
    CHECK(is_true(value(false) || value(true)));
    CHECK(is_false(value(false) || value(false)));

    CHECK_FALSE(signal_has_value(empty<bool>() || empty<bool>()));
    CHECK_FALSE(signal_has_value(value(false) || empty<bool>()));
    CHECK_FALSE(signal_has_value(empty<bool>() || value(false)));
    CHECK(is_true(value(true) || empty<bool>()));
    CHECK(is_true(empty<bool>() || value(true)));

    int access_count = 0;
    CHECK(is_true(value(true) || counting_bool(&access_count, false)));
    CHECK(access_count == 0);

    CHECK(
        ((value(false) || value(false)).value_id()
         != (value(true) || value(false)).value_id()));

    CHECK(is_true(true || value(false)));
    CHECK(is_true(value(false) || true));
    CHECK(is_false(false || value(false)));
    CHECK(is_false(value(false) || false));
}

TEST_CASE("conditional")
{
    bool condition = false;
    auto s = conditional(ref(condition), value(1), value(2));

    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 2);
    condition = true;
    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1);

    CHECK(
        (conditional(value(false), value(2), value(2)).value_id()
         != conditional(value(true), value(2), value(2)).value_id()));
}

TEST_CASE("non-boolean conditional")
{
    CHECK(is_true(conditional(value(2), value(1), value(0)) == value(1)));
    CHECK(is_true(conditional(value(0), value(1), value(0)) == value(0)));
}

TEST_CASE("conditional with different capabilities")
{
    bool condition = false;
    auto s = conditional(ref(condition), empty<int>(), value(2));

    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 2);
    condition = true;
    CHECK_FALSE(signal_has_value(s));
}

TEST_CASE("conditional with empty condition")
{
    int x = 0, y = 1;
    auto s = conditional(empty<bool>(), ref(x), ref(y));
    CHECK_FALSE(signal_has_value(s));
    CHECK((s.value_id() == null_id()));
    CHECK_FALSE(signal_ready_to_write(s));
}

TEST_CASE("writable conditional")
{
    bool condition = false;
    int x = 1;
    int y = 2;
    auto s = conditional(ref(condition), ref(x), ref(y));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 2);
    condition = true;
    CHECK(read_signal(s) == 1);
    write_signal(s, 4);
    CHECK(x == 4);
    CHECK(y == 2);
    CHECK(read_signal(s) == 4);
    condition = false;
    write_signal(s, 3);
    CHECK(x == 4);
    CHECK(y == 3);
    CHECK(read_signal(s) == 3);
}

TEST_CASE("field signal")
{
    struct foo
    {
        int x;
        std::string y;
    };

    state_storage<foo> storage;
    alia_context ctx{};
    auto f_signal = make_state_binding(storage, &ctx);
    write_signal(f_signal, foo{2, "1.5"});

    auto x_signal = f_signal->*&foo::x;

    static_assert(std::same_as<decltype(x_signal)::value_type, int>);
    static_assert(view_signal<decltype(x_signal)>);
    static_assert(sink_signal<decltype(x_signal)>);

    CHECK(signal_has_value(x_signal));
    CHECK(read_signal(x_signal) == 2);
    CHECK(signal_ready_to_write(x_signal));
    auto original_x_id = x_signal.value_id();
    write_signal(x_signal, 1);
    CHECK(storage.value.x == 1);
    CHECK(read_signal(x_signal) == 1);
    CHECK((x_signal.value_id() != original_x_id));

    auto y_signal = alia_field(f_signal, y);

    static_assert(std::same_as<decltype(y_signal)::value_type, std::string>);
    static_assert(view_signal<decltype(y_signal)>);
    static_assert(sink_signal<decltype(y_signal)>);

    CHECK((y_signal.value_id() != x_signal.value_id()));
    CHECK(signal_has_value(y_signal));
    CHECK(read_signal(y_signal) == "1.5");
    CHECK(signal_ready_to_write(y_signal));
    write_signal(y_signal, "0.5");
    CHECK(storage.value.y == "0.5");
    CHECK(read_signal(y_signal) == "0.5");
}

TEST_CASE("read-only field signal")
{
    struct point
    {
        int x;
        int y;
    };

    auto s = value(point{3, 4})->*&point::x;

    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 3);
}

TEST_CASE("empty field signal")
{
    struct foo
    {
        int x;
        std::string y;
    };

    auto s = empty<foo>()->*&foo::x;
    CHECK_FALSE(signal_has_value(s));
    CHECK((s.value_id() == null_id()));
    CHECK_FALSE(signal_ready_to_write(s));
}

TEST_CASE("field signal with non-identifiable field")
{
    struct inner
    {
        std::string name;
        int n;
    };
    struct outer
    {
        inner first;
        inner second;
    };

    static_assert(!identifiable<inner>);

    state_storage<outer> storage;
    alia_context ctx{};
    auto s = make_state_binding(storage, &ctx);
    write_signal(s, outer{{"a", 1}, {"b", 2}});

    auto first = s->*&outer::first;
    auto second = alia_field(s, second);

    CHECK(signal_has_value(first));
    CHECK(read_signal(first).n == 1);
    CHECK((first.value_id() != second.value_id()));

    write_signal(first, inner{"c", 3});
    CHECK(storage.value.first.name == "c");
    CHECK(storage.value.first.n == 3);
    CHECK(read_signal(second).n == 2);
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

TEST_CASE("subscript metafunctions")
{
    static_assert(has_at_indexer<std::vector<int>, int>);
    static_assert(has_at_indexer<std::map<int, int>, int>);
    static_assert(has_at_indexer<std::vector<bool>, int>);
    static_assert(!has_at_indexer<my_array, int>);
    static_assert(!has_at_indexer<my_const_array, int>);

    static_assert(std::same_as<
                  subscript_result_type<std::vector<float>, int>::type,
                  float>);
    static_assert(std::same_as<
                  subscript_result_type<std::map<int, float>, int>::type,
                  float>);
    static_assert(std::same_as<
                  subscript_result_type<std::vector<bool>, int>::type,
                  bool>);
    static_assert(
        std::same_as<subscript_result_type<my_array, int>::type, int>);
    static_assert(
        std::same_as<subscript_result_type<my_const_array, int>::type, int>);

    static_assert(const_subscript_returns_reference<std::vector<int>, int>);
    static_assert(const_subscript_returns_reference<std::map<int, int>, int>);
    static_assert(
        !const_subscript_returns_reference<std::vector<bool>, int>);
    static_assert(const_subscript_returns_reference<my_array, int>);
    static_assert(!const_subscript_returns_reference<my_const_array, int>);
}

TEST_CASE("vector subscript")
{
    std::vector<int> c{2, 0, 3};
    uint32_t version = 0;
    auto c_signal = versioned_ref(c, version);
    auto s = c_signal[value(1)];

    static_assert(std::same_as<decltype(s)::value_type, int>);
    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 0);
    CHECK(signal_ready_to_write(s));
    auto original_id = s.value_id();
    write_signal(s, 1);
    CHECK((c == std::vector<int>{2, 1, 3}));
    CHECK((s.value_id() != original_id));

    auto t = c_signal[value(0)];
    CHECK((t.value_id() != s.value_id()));
}

TEST_CASE("read-only subscript")
{
    std::vector<int> const c{2, 0, 3};
    uint32_t const version = 1;
    auto s = versioned_ref(c, version)[value(1)];

    static_assert(std::same_as<decltype(s)::value_type, int>);
    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 0);
}

TEST_CASE("subscript with raw index")
{
    std::vector<int> const c{2, 0, 3};
    uint32_t const version = 1;
    auto s = versioned_ref(c, version)[1];

    static_assert(std::same_as<decltype(s)::value_type, int>);
    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 0);
}

TEST_CASE("vector<bool> subscript")
{
    std::vector<bool> c{true, false, false};
    uint32_t version = 0;
    auto s = versioned_ref(c, version)[value(1)];

    static_assert(std::same_as<decltype(s)::value_type, bool>);
    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == false);
    CHECK(signal_ready_to_write(s));
    write_signal(s, true);
    CHECK((c == std::vector<bool>{true, true, false}));
}

TEST_CASE("map subscript")
{
    std::map<int, int> c{{2, 1}, {0, 3}};
    uint32_t version = 0;
    auto s = versioned_ref(c, version)[value(2)];

    static_assert(std::same_as<decltype(s)::value_type, int>);
    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1);
    CHECK(signal_ready_to_write(s));
    write_signal(s, 7);
    CHECK((c == std::map<int, int>{{2, 7}, {0, 3}}));
}

TEST_CASE("custom ref subscript")
{
    my_array c;
    uint32_t version = 0;
    auto s = versioned_ref(c, version)[value(2)];

    static_assert(std::same_as<decltype(s)::value_type, int>);
    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 3);
    CHECK(signal_ready_to_write(s));
    write_signal(s, 4);
    CHECK(c[2] == 4);
}

TEST_CASE("custom by-value subscript")
{
    my_const_array const c;
    uint32_t const version = 1;
    auto s = versioned_ref(c, version)[value(2)];

    static_assert(std::same_as<decltype(s)::value_type, int>);
    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 3);
}

TEST_CASE("empty subscript")
{
    auto s = empty<std::map<int, int>>()[value(2)];
    CHECK_FALSE(signal_has_value(s));
    CHECK((s.value_id() == null_id()));
}

TEST_CASE("subscript with non-identifiable element")
{
    struct inner
    {
        std::string name;
        int n;
    };

    static_assert(!identifiable<inner>);

    std::vector<inner> c{{"a", 1}, {"b", 2}};
    uint32_t version = 0;
    auto container = versioned_ref(c, version);
    auto first = container[0];
    auto second = container[1];

    CHECK(signal_has_value(first));
    CHECK(read_signal(first).n == 1);
    CHECK((first.value_id() != second.value_id()));

    write_signal(first, inner{"c", 3});
    CHECK(c[0].name == "c");
    CHECK(c[0].n == 3);
    CHECK(read_signal(second).n == 2);
}
