#include <alia/kernel/signals/operators.hpp>

#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/state.hpp>

#include <doctest/doctest.h>

#include <string>

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
