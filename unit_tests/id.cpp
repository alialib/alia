#include <alia/id.hpp>

#include <map>
#include <unordered_map>
#include <utility>

#include <testing.hpp>

using namespace alia;

// Test all the relevant ID operations on a pair of equal IDs.
static void
test_equal_ids(id_interface const& a, id_interface const& b)
{
    REQUIRE(a == b);
    REQUIRE(b == a);
    REQUIRE(!(a < b));
    REQUIRE(!(b < a));
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

TEST_CASE("captured_id basics", "[id]")
{
    captured_id c;
    REQUIRE(!c.is_initialized());
    REQUIRE(!c.matches(make_id(0)));
    c.capture(make_id(0));
    REQUIRE(c.is_initialized());
    REQUIRE(c.matches(make_id(0)));
    REQUIRE(!c.matches(make_id(1)));
    REQUIRE(c.get() == make_id(0));
    c.clear();
    REQUIRE(!c.is_initialized());
}

TEST_CASE("captured_id operators", "[id]")
{
    captured_id c;
    c.capture(make_id(0));
    captured_id d;
    REQUIRE(c != d);
    d.capture(make_id(0));
    REQUIRE(c == d);
    d.capture(make_id(1));
    REQUIRE(c != d);
    REQUIRE(c < d);
}

TEST_CASE("captured_id copy construction", "[id]")
{
    captured_id c;
    c.capture(make_id(0));
    captured_id d = c;
    REQUIRE(d == c);
    // Check that d is independent of changes in c.
    c.capture(make_id(1));
    REQUIRE(d != c);

    c.clear();
    captured_id e = c;
    REQUIRE(e == c);
    // Check that e is independent of changes in c.
    c.capture(make_id(1));
    REQUIRE(e != c);
}

TEST_CASE("captured_id move construction", "[id]")
{
    captured_id c;
    c.capture(make_id(0));
    captured_id d = std::move(c);
    REQUIRE(d.matches(make_id(0)));

    captured_id e;
    e.clear();
    captured_id f = std::move(e);
    REQUIRE(!f.is_initialized());
}

TEST_CASE("captured_id copy assignment", "[id]")
{
    captured_id c;
    c.capture(make_id(0));
    captured_id d;
    d = c;
    REQUIRE(d == c);
    // Check that d is independent of changes in c.
    c.capture(make_id(1));
    REQUIRE(d != c);

    c.clear();
    captured_id e;
    e = c;
    REQUIRE(e == c);
    // Check that e is independent of changes in c.
    c.capture(make_id(1));
    REQUIRE(e != c);
}

TEST_CASE("captured_id move assignment", "[id]")
{
    captured_id c;
    c.capture(make_id(0));
    captured_id d;
    d = std::move(c);
    REQUIRE(d.matches(make_id(0)));

    captured_id e;
    e.clear();
    captured_id f;
    f = std::move(e);
    REQUIRE(!f.is_initialized());
}

TEST_CASE("combine_ids x1", "[id]")
{
    auto a = combine_ids(make_id(0));
    auto b = combine_ids(make_id(1));
    test_different_ids(a, b);
}

TEST_CASE("combine_ids x2", "[id]")
{
    auto a = combine_ids(make_id(0), make_id(1));
    auto b = combine_ids(make_id(1), make_id(2));
    test_different_ids(a, b);
}

TEST_CASE("combine_ids x3", "[id]")
{
    auto a = combine_ids(make_id(0), make_id(1), make_id(2));
    auto b = combine_ids(make_id(1), make_id(2), make_id(3));
    test_different_ids(a, b);
}

TEST_CASE("combine_ids x4", "[id]")
{
    auto a = combine_ids(make_id(0), make_id(1), make_id(2), make_id(3));
    auto b = combine_ids(make_id(1), make_id(2), make_id(3), make_id(4));
    test_different_ids(a, b);
}

TEST_CASE("clone_into/pointer", "[id]")
{
    id_interface* storage = 0;
    auto zero = make_id(0);
    auto abc = make_id(std::string("abc"));
    auto one = make_id(1);
    clone_into(storage, &zero);
    REQUIRE(*storage == zero);
    clone_into(storage, &one);
    REQUIRE(*storage == one);
    clone_into(storage, &abc);
    REQUIRE(*storage == abc);
    clone_into(storage, nullptr);
    REQUIRE(!storage);
}

TEST_CASE("clone_into/shared_ptr", "[id]")
{
    std::shared_ptr<id_interface> storage;
    auto zero = make_id(0);
    auto abc = make_id(std::string("abc"));
    auto one = make_id(1);
    clone_into(storage, &zero);
    REQUIRE(*storage == zero);
    clone_into(storage, &one);
    REQUIRE(*storage == one);
    clone_into(storage, &abc);
    REQUIRE(*storage == abc);
    clone_into(storage, nullptr);
    REQUIRE(!storage);
}

TEST_CASE("map of IDs", "[id]")
{
    auto zero = make_id(0);
    auto abc = make_id(std::string("abc"));
    auto one = make_id(1);
    auto another_one = make_id(1);

    std::map<id_interface const*, int, id_interface_pointer_less_than_test> m;
    m[&zero] = 0;
    m[&abc] = 123;
    REQUIRE(m.at(&zero) == 0);
    REQUIRE(m.at(&abc) == 123);
    REQUIRE(m.find(&one) == m.end());
    m[&one] = 1;
    REQUIRE(m.at(&one) == 1);
    REQUIRE(m.at(&another_one) == 1);
}
