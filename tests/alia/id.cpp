#include <alia/id.hpp>
#include <utility>
#include <boost/lexical_cast.hpp>

#define BOOST_TEST_MODULE id
#include "test.hpp"

BOOST_AUTO_TEST_CASE(id_test)
{
    using namespace alia;

    // Test value IDs and the basic ID interface operators.
    value_id<int> a = make_id(1);
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a), "1");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(ref(a)), "1");
    BOOST_CHECK(get_context(a) == ID_CONTEXT_UNIVERSAL);
    BOOST_CHECK(get_context(ref(a)) == ID_CONTEXT_UNIVERSAL);
    BOOST_CHECK(a == a);
    value_id<int> c = make_id(1);
    BOOST_CHECK(ref(a) == ref(a));
    BOOST_CHECK(a == c);
    BOOST_CHECK(ref(a) == ref(c));
    value_id<int> b = make_id(2);
    BOOST_CHECK(a != b);
    BOOST_CHECK(ref(a) != ref(b));
    BOOST_CHECK(a < b);
    BOOST_CHECK(ref(a) < ref(b));
    BOOST_CHECK(!(b < a));
    BOOST_CHECK(!(ref(b) < ref(a)));
    int x;
    value_id<int*> d = make_id(&x);
    BOOST_CHECK(get_context(d) == ID_CONTEXT_APP_INSTANCE);
    BOOST_CHECK(a != d);
    BOOST_CHECK(ref(a) != ref(d));
    BOOST_CHECK(a < d && !(d < a) || d < a && !(a < d));

    // Test owned_id.
    owned_id o;
    o.store(a);
    BOOST_CHECK(o.get() == a);
    BOOST_CHECK(o.get() != b);
    owned_id p;
    BOOST_CHECK(o != p);
    p.store(a);
    BOOST_CHECK(o == p);
    p.store(c);
    BOOST_CHECK(o == p);
    p.store(b);
    BOOST_CHECK(o != p);
    BOOST_CHECK(o < p);
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(o), "1");

    // Test id_pair.
    o.store(combine_ids(a, b));
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(o), "(1,2)");
    BOOST_CHECK(o.get() == combine_ids(a, b));
    BOOST_CHECK(combine_ids(a, c) < combine_ids(a, b));
    BOOST_CHECK(combine_ids(a, b) != combine_ids(b, a));
    BOOST_CHECK(combine_ids(a, b) < combine_ids(b, a));
    o.store(combine_ids(a, ref(b)));
    BOOST_CHECK(o.get() == combine_ids(a, ref(b)));
}
