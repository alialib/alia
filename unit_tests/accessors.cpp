#if 0

#include <alia/accessors.hpp>
#include <string>

#include <catch.hpp>

template<class T>
T const&
set_and_get(alia::accessor<T> const& x, T const& value)
{
    x.set(value);
    return x.get();
}

template<class T>
bool
is_readonly(alia::accessor<T> const& x)
{
    return x.is_gettable() && !x.is_settable();
}

void
test_ref(alia::accessor<int> const& x)
{
    x.set(0);
    REQUIRE(set_and_get(ref(x), 3) == 3);
    REQUIRE(x.get() == 3);
}

struct foo
{
    foo() : x(0), y(1)
    {
    }
    int x;
    double y;
};
bool
operator==(foo a, foo b)
{
    return a.x == b.x && a.y == b.y;
}
bool
operator!=(foo a, foo b)
{
    return !(a == b);
}
bool
operator<(foo a, foo b)
{
    return a.x < b.x || a.x == b.x && a.y < b.y;
}
std::ostream&
operator<<(std::ostream& s, foo f)
{
    s << "(" << f.x << "," << f.y << ")";
    return s;
}
namespace std {
template<>
struct hash<foo>
{
    size_t
    operator()(foo const& f) const
    {
        return 0;
    }
};
} // namespace std

TEST_CASE("accessors", "[accessors]")
{
    using namespace alia;

    // test input accessors
    REQUIRE(is_readonly(in(1)));
    REQUIRE(get(in(1)) == 1);

    // test state_proxy
    state_proxy<int> s;
    REQUIRE(!s.is_initialized());
    REQUIRE(!s.was_set());
    s.initialize(0);
    REQUIRE(s.is_initialized());
    REQUIRE(!s.was_set());
    REQUIRE(get(make_accessor(s)) == 0);
    REQUIRE(set_and_get(make_accessor(s), 1) == 1);
    REQUIRE(s.was_set());
    state_proxy<int> t(2);
    REQUIRE(t.is_initialized());
    REQUIRE(!t.was_set());
    REQUIRE(get(make_accessor(t)) == 2);

    // test inout and make_readonly
    int x = 1;
    REQUIRE(get(inout(&x)) == 1);
    REQUIRE(set_and_get(inout(&x), 2) == 2);
    REQUIRE(x == 2);
    REQUIRE(is_readonly(make_readonly(inout(&x))));

    // test casting
    double y = 0;
    REQUIRE(set_and_get(accessor_cast<int>(inout(&y)), 1) == 1);
    REQUIRE(y == 1.0);

    // test select_accessor
    REQUIRE(
        get(select_accessor(in(true), inout(&x), accessor_cast<int>(inout(&y))))
        == 2);
    REQUIRE(
        get(select_accessor(
            in(false), inout(&x), accessor_cast<int>(inout(&y))))
        == 1);
    REQUIRE(!is_readonly(select_accessor(in(true), inout(&x), in(0))));
    REQUIRE(is_readonly(select_accessor(in(false), inout(&x), in(0))));

    // test ref
    test_ref(inout(&x));
    test_ref(make_accessor(s));

    // test scaling
    x = 1;
    REQUIRE(get(scale(inout(&x), 10)) == 10);
    set(scale(inout(&x), 10), 20);
    REQUIRE(x == 2);
    REQUIRE(set_and_get(scale(inout(&x), 10), 40) == 40);
    REQUIRE(x == 4);

    // test offset
    x = 1;
    REQUIRE(get(offset(inout(&x), 10)) == 11);
    set(offset(inout(&x), 10), 20);
    REQUIRE(x == 10);
    REQUIRE(set_and_get(offset(inout(&x), 10), 40) == 40);
    REQUIRE(x == 30);

    // test rounding
    y = 1;
    set(add_input_rounder(inout(&y), 0.5), 1.4);
    REQUIRE(y == 1.5);
    REQUIRE(set_and_get(add_input_rounder(inout(&y), 0.5), 0.9) == 1.);
    REQUIRE(y == 1.);

    // test select_field
    foo f;
    f.x = 4;
    REQUIRE(get(select_field(inout(&f), &foo::x)) == 4);
    REQUIRE(set_and_get(select_field(inout(&f), &foo::x), 0) == 0);
    REQUIRE(f.x == 0);
    REQUIRE(set_and_get(select_field(inout(&f), &foo::y), 2.) == 2.);
    REQUIRE(f.y == 2.);
}

#endif
