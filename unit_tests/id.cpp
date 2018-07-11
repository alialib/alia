#include <alia/id.hpp>
#include <boost/lexical_cast.hpp>
#include <utility>

#include <catch.hpp>

using namespace alia;

// Test all the relevant ID operations on a pair of equal IDs.
static void
test_equal_ids(id_interface const& a, id_interface const& b)
{
    REQUIRE(a == b);
    REQUIRE(b == a);
    REQUIRE(!(a < b));
    REQUIRE(!(b < a));
    REQUIRE(
        boost::lexical_cast<std::string>(a)
        == boost::lexical_cast<std::string>(b));
    REQUIRE(a.hash() == b.hash());
}

// Test all the ID operations on a single ID.
template<class Id>
void
test_single_id(Id const& id)
{
    test_equal_ids(id, id);

    std::shared_ptr<id_interface> clone(id.clone());
    test_equal_ids(id, *clone);

    Id copy;
    id.deep_copy(&copy);
    test_equal_ids(id, copy);

    // Copying a clone is sometimes different because the clone is free of
    // references to the surrounding stack frame.
    Id clone_copy;
    clone->deep_copy(&clone_copy);
    test_equal_ids(id, clone_copy);
}

// Test all the ID operations on a pair of different IDs.
template<class A, class B>
void
test_different_ids(A const& a, B const& b)
{
    test_single_id(a);
    test_single_id(b);
    REQUIRE(a != b);
    REQUIRE((a < b && !(b < a) || b < a && !(a < b)));
    REQUIRE(a.hash() != b.hash());
}

TEST_CASE("simple_id", "[id]")
{
    test_different_ids(make_id(0), make_id(1));
}

TEST_CASE("simple_id_by_reference", "[id]")
{
    int x = 0, y = 1;
    test_different_ids(make_id_by_reference(x), make_id_by_reference(y));
}

TEST_CASE("id_ref", "[id]")
{
    test_different_ids(ref(make_id(0)), ref(make_id(1)));
}

TEST_CASE("captured_id", "[id]")
{
    captured_id c;
    c.capture(make_id(0));
    REQUIRE(c.matches(make_id(0)));
    REQUIRE(!c.matches(make_id(1)));
    REQUIRE(c.get() == make_id(0));
    captured_id d;
    REQUIRE(c != d);
    d.capture(make_id(0));
    REQUIRE(c == d);
    d.capture(make_id(1));
    REQUIRE(c != d);
    REQUIRE(c < d);
    REQUIRE(boost::lexical_cast<std::string>(c) == "0");
}

TEST_CASE("id_pair", "[id]")
{
    auto a = combine_ids(make_id(0), make_id(1));
    auto b = combine_ids(make_id(1), make_id(2));
    REQUIRE(boost::lexical_cast<std::string>(a) == "(0,1)");
    REQUIRE(boost::lexical_cast<std::string>(b) == "(1,2)");
    test_different_ids(a, b);
}
