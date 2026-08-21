#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

#include <stdint.h>
#include <string>
#include <vector>

using namespace alia;
using namespace alia::operators;

TEST_CASE("value and read")
{
    auto s = value(42);
    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 42);
    CHECK(s.value_id() == 42);
    static_assert(nonempty_view_signal<decltype(s)>);
}

TEST_CASE("empty signal")
{
    auto s = empty<int>();
    CHECK_FALSE(signal_has_value(s));
    CHECK(
        (static_cast<untyped_signal_base const&>(s).value_id_view()
         == null_id()));
    static_assert(!nonempty_view_signal<decltype(s)>);
    static_assert(view_signal<decltype(s)>);
}

TEST_CASE("ref binding write")
{
    int x = 1;
    auto s = ref(x);
    static_assert(nonempty_binding_signal<decltype(s)>);
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
    CHECK(
        (static_cast<untyped_signal_base const&>(s).value_id_view()
         == make_pointer_id("hello")));
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
    CHECK(std::is_same_v<decltype(s.value_id()), constant_value_tag>);
}

TEST_CASE("versioned_ref binding")
{
    std::vector<int> items{1, 2};
    uint32_t version = 0;
    auto s = versioned_ref(items, version);

    static_assert(!identifiable<std::vector<int>>);
    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK((read_signal(s) == std::vector<int>{1, 2}));
    CHECK(s.value_id() == 0u);

    write_signal(s, std::vector<int>{3, 4, 5});
    CHECK((items == std::vector<int>{3, 4, 5}));
    CHECK(version == 1);
    CHECK(s.value_id() == 1u);

    version = 7;
    CHECK(s.value_id() == 7u);
}

TEST_CASE("versioned_ref const view")
{
    std::vector<int> const items{1, 2};
    uint32_t const version = 5;
    auto s = versioned_ref(items, version);

    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK((read_signal(s) == std::vector<int>{1, 2}));
    CHECK(s.value_id() == 5u);
}
