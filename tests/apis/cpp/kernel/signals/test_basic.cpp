#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

#include <string>

using namespace alia;
using namespace alia::operators;

TEST_CASE("value and read")
{
    auto s = value(42);
    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 42);
    CHECK((s.value_id() == make_id(42)));
}

TEST_CASE("empty signal")
{
    auto s = empty<int>();
    CHECK_FALSE(signal_has_value(s));
    CHECK((s.value_id() == null_id()));
}

TEST_CASE("ref binding write")
{
    int x = 1;
    auto s = ref(x);
    CHECK(read_signal(s) == 1);
    write_signal(s, 7);
    CHECK(x == 7);
    CHECK(read_signal(s) == 7);
}

TEST_CASE("ref const view")
{
    int const x = 3;
    auto s = ref(x);
    CHECK(read_signal(s) == 3);
    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);
}

TEST_CASE("string literal value")
{
    auto s = value("hello");
    CHECK(read_signal(s) == std::string("hello"));
    CHECK((s.value_id() == make_pointer_id("hello")));
}

TEST_CASE("signalize")
{
    auto a = signalize(5);
    CHECK(read_signal(a) == 5);

    auto b = signalize(value(9));
    CHECK(read_signal(b) == 9);
}

TEST_CASE("erased view and binding")
{
    auto owned = value(11);
    view<int> v = owned;
    CHECK(read_signal(v) == 11);

    int x = 2;
    auto r = ref(x);
    binding<int> b = r;
    write_signal(b, 4);
    CHECK(x == 4);
}

TEST_CASE("default_initialized")
{
    auto s = default_initialized<int>();
    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 0);
    CHECK((s.value_id() == unit_id()));
}
