#include <alia/accessors.hpp>
#include <string>

#define BOOST_TEST_MODULE accessors
#include "test.hpp"

template<class T>
T const& set_and_get(alia::accessor<T> const& x, T const& value)
{
    set(x, value);
    return get(x);
}

template<class T>
bool is_readonly(alia::accessor<T> const& x)
{
    return x.is_gettable() && !x.is_settable();
}

void test_ref(alia::accessor<int> const& x)
{
    set(x, 0);
    BOOST_CHECK_EQUAL(set_and_get(ref(x), 3), 3);
    BOOST_CHECK_EQUAL(get(x), 3);
}

struct foo
{
    foo() : x(0), y(1) {}
    int x;
    double y;
};
bool operator==(foo a, foo b)
{ return a.x == b.x && a.y == b.y; }
bool operator!=(foo a, foo b)
{ return !(a == b); }
bool operator<(foo a, foo b)
{ return a.x < b.x || a.x == b.x && a.y < b.y; }
std::ostream& operator<<(std::ostream& s, foo f)
{
    s << "(" << f.x << "," << f.y << ")";
    return s;
}

BOOST_AUTO_TEST_CASE(accessors)
{
    using namespace alia;

    // test input accessors
    BOOST_CHECK(is_readonly(in(1)));
    BOOST_CHECK_EQUAL(get(in(1)), 1);

    // test state_proxy
    state_proxy<int> s;
    BOOST_CHECK(!s.is_initialized());
    BOOST_CHECK(!s.was_set());
    s.initialize(0);
    BOOST_CHECK(s.is_initialized());
    BOOST_CHECK(!s.was_set());
    BOOST_CHECK_EQUAL(get(make_accessor(s)), 0);
    BOOST_CHECK_EQUAL(set_and_get(make_accessor(s), 1), 1);
    BOOST_CHECK(s.was_set());
    state_proxy<int> t(2);
    BOOST_CHECK(t.is_initialized());
    BOOST_CHECK(!t.was_set());
    BOOST_CHECK_EQUAL(get(make_accessor(t)), 2);

    // test inout and make_readonly
    int x = 1;
    BOOST_CHECK_EQUAL(get(inout(&x)), 1);
    BOOST_CHECK_EQUAL(set_and_get(inout(&x), 2), 2);
    BOOST_CHECK_EQUAL(x, 2);
    BOOST_CHECK(is_readonly(make_readonly(inout(&x))));

    // test casting
    double y = 0;
    BOOST_CHECK_EQUAL(set_and_get(accessor_cast<int>(inout(&y)), 1), 1);
    BOOST_CHECK_EQUAL(y, 1.0);

    // test select_accessor
    BOOST_CHECK_EQUAL(get(select_accessor(true, inout(&x),
        accessor_cast<int>(inout(&y)))), 2);
    BOOST_CHECK_EQUAL(get(select_accessor(false, inout(&x),
        accessor_cast<int>(inout(&y)))), 1);
    BOOST_CHECK(!is_readonly(select_accessor(true, inout(&x), in(0))));
    BOOST_CHECK(is_readonly(select_accessor(false, inout(&x), in(0))));

    // test ref
    test_ref(inout(&x));
    test_ref(make_accessor(s));

    // test scaling
    x = 1;
    BOOST_CHECK_EQUAL(get(scale(inout(&x), 10)), 10);
    set(scale(inout(&x), 10), 20);
    BOOST_CHECK_EQUAL(x, 2);
    BOOST_CHECK_EQUAL(set_and_get(scale(inout(&x), 10), 40), 40);
    BOOST_CHECK_EQUAL(x, 4);

    // test offset
    x = 1;
    BOOST_CHECK_EQUAL(get(offset(inout(&x), 10)), 11);
    set(offset(inout(&x), 10), 20);
    BOOST_CHECK_EQUAL(x, 10);
    BOOST_CHECK_EQUAL(set_and_get(offset(inout(&x), 10), 40), 40);
    BOOST_CHECK_EQUAL(x, 30);

    // test rounding
    y = 1;
    set(add_input_rounder(inout(&y), 0.5), 1.4);
    BOOST_CHECK_EQUAL(y, 1.5);
    BOOST_CHECK_EQUAL(set_and_get(add_input_rounder(inout(&y), 0.5), 0.9), 1.);
    BOOST_CHECK_EQUAL(y, 1.);

    // test select_field
    foo f;
    f.x = 4;
    BOOST_CHECK_EQUAL(get(select_field(inout(&f), &foo::x)), 4);
    BOOST_CHECK_EQUAL(set_and_get(select_field(inout(&f), &foo::x), 0), 0);
    BOOST_CHECK_EQUAL(f.x, 0);
    BOOST_CHECK_EQUAL(set_and_get(select_field(inout(&f), &foo::y), 2.), 2.);
    BOOST_CHECK_EQUAL(f.y, 2.);
}
