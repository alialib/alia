#include <alia/id.hpp>
#include <boost/lexical_cast.hpp>
#include <utility>

#include <catch.hpp>

static void
test_ids(alia::id_interface const& a, alia::id_interface const& b)
{
}

TEST_CASE("IDs", "[id]")
{
    using namespace alia;

    // Test simple IDs and the basic ID interface operators.
    simple_id<int> a = make_id(1);
    REQUIRE(boost::lexical_cast<std::string>(a) == "1");
    REQUIRE(boost::lexical_cast<std::string>(ref(a)) == "1");
    REQUIRE(a == a);
    simple_id<int> c = make_id(1);
    REQUIRE(ref(a) == ref(a));
    REQUIRE(a == c);
    REQUIRE(ref(a) == ref(c));
    simple_id<int> b = make_id(2);
    REQUIRE(a != b);
    REQUIRE(ref(a) != ref(b));
    REQUIRE(a < b);
    REQUIRE(ref(a) < ref(b));
    REQUIRE(!(b < a));
    REQUIRE(!(ref(b) < ref(a)));
    int x;
    simple_id<int*> d = make_id(&x);
    REQUIRE(a != d);
    REQUIRE(ref(a) != ref(d));
    REQUIRE((a < d && !(d < a) || d < a && !(a < d)));

    // Test captured_id.
    captured_id o;
    o.capture(a);
    REQUIRE(o.get() == a);
    REQUIRE(o.get() != b);
    captured_id p;
    REQUIRE(o != p);
    p.capture(a);
    REQUIRE(o == p);
    p.capture(c);
    REQUIRE(o == p);
    p.capture(b);
    REQUIRE(o != p);
    REQUIRE(o < p);
    REQUIRE(boost::lexical_cast<std::string>(o) == "1");

    // Test id_pair.
    o.capture(combine_ids(a, b));
    REQUIRE(boost::lexical_cast<std::string>(o) == "(1,2)");
    REQUIRE(o.get() == combine_ids(a, b));
    REQUIRE(combine_ids(a, c) < combine_ids(a, b));
    REQUIRE(combine_ids(a, b) != combine_ids(b, a));
    REQUIRE(combine_ids(a, b) < combine_ids(b, a));
    o.capture(combine_ids(a, ref(b)));
    REQUIRE(o.get() == combine_ids(a, ref(b)));
}
