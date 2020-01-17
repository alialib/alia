#include <alia/components/typing.hpp>

#include <catch.hpp>

#include <boost/any.hpp>

using namespace alia;

// component_collection.hpp is supposed to define this by default.
#ifndef ALIA_STATIC_COMPONENT_CHECKING
#error ALIA_STATIC_COMPONENT_CHECKING not defined
#endif

// Define some arbitrary tag and data types.
struct foo
{
    bool b = false;
};
struct foo_tag
{
};

struct bar
{
    int i = 0;
    bar()
    {
    }
    bar(int i) : i(i)
    {
    }
};
struct bar_tag
{
};

struct zap
{
    double d = 0;
};
struct zap_tag
{
};

// Test the underlying mechanics of adding and removing tags in lists.
namespace list_tests {
using namespace detail;
using list_empty = tag_list<>;
static_assert(!list_contains_tag<list_empty, foo_tag>::value, "");
using list_b = add_tag_to_list<list_empty, bar_tag>::type;
static_assert(!list_contains_tag<list_b, foo_tag>::value, "");
static_assert(list_contains_tag<list_b, bar_tag>::value, "");
using list_fb = add_tag_to_list<list_b, foo_tag>::type;
static_assert(list_contains_tag<list_fb, foo_tag>::value, "");
static_assert(list_contains_tag<list_fb, bar_tag>::value, "");
using list_f = remove_tag_from_list<list_fb, bar_tag>::type;
static_assert(list_contains_tag<list_f, foo_tag>::value, "");
static_assert(!list_contains_tag<list_f, bar_tag>::value, "");
using list_zfb = add_tag_to_list<list_fb, zap_tag>::type;
static_assert(list_contains_tag<list_zfb, foo_tag>::value, "");
static_assert(list_contains_tag<list_zfb, bar_tag>::value, "");
static_assert(list_contains_tag<list_zfb, zap_tag>::value, "");
using list_zb = remove_tag_from_list<list_zfb, foo_tag>::type;
static_assert(!list_contains_tag<list_zb, foo_tag>::value, "");
static_assert(list_contains_tag<list_zb, bar_tag>::value, "");
static_assert(list_contains_tag<list_zb, zap_tag>::value, "");
using list_z = remove_tag_from_list<list_zb, bar_tag>::type;
static_assert(!list_contains_tag<list_z, foo_tag>::value, "");
static_assert(!list_contains_tag<list_z, bar_tag>::value, "");
static_assert(list_contains_tag<list_z, zap_tag>::value, "");
using list_fbz = merge_tag_lists<list_fb, list_zb>::type;
static_assert(list_contains_tag<list_fbz, foo_tag>::value, "");
static_assert(list_contains_tag<list_fbz, bar_tag>::value, "");
static_assert(list_contains_tag<list_fbz, zap_tag>::value, "");
using list_bf = merge_tag_lists<list_b, list_f>::type;
static_assert(list_contains_tag<list_bf, foo_tag>::value, "");
static_assert(list_contains_tag<list_bf, bar_tag>::value, "");
static_assert(!list_contains_tag<list_bf, zap_tag>::value, "");
using list_b_ = merge_tag_lists<list_b, list_b>::type;
static_assert(!list_contains_tag<list_b_, foo_tag>::value, "");
static_assert(list_contains_tag<list_b_, bar_tag>::value, "");
static_assert(!list_contains_tag<list_b_, zap_tag>::value, "");
using list_f_ = merge_tag_lists<list_f, list_empty>::type;
static_assert(list_contains_tag<list_f_, foo_tag>::value, "");
static_assert(!list_contains_tag<list_f_, bar_tag>::value, "");
static_assert(!list_contains_tag<list_f_, zap_tag>::value, "");
using list_z_ = merge_tag_lists<list_empty, list_z>::type;
static_assert(!list_contains_tag<list_z_, foo_tag>::value, "");
static_assert(!list_contains_tag<list_z_, bar_tag>::value, "");
static_assert(list_contains_tag<list_z_, zap_tag>::value, "");
} // namespace list_tests

// Define some arbitrary component collection types.
using storage_type = generic_component_storage<boost::any>;
using cc_empty = empty_component_collection<storage_type>;
using cc_b = add_component_type_t<cc_empty, bar_tag>;
using cc_fb = add_component_type_t<cc_b, foo_tag>;
using cc_z = add_component_type_t<cc_empty, zap_tag>;
using cc_bz = add_component_type_t<cc_z, bar_tag>;
using cc_fbz = add_component_type_t<cc_bz, foo_tag>;
using cc_fz = remove_component_type_t<cc_fbz, bar_tag>;
using cc_f = add_component_type_t<cc_empty, foo_tag>;
using cc_fzb = merge_components_t<cc_fz, cc_bz>;

// Test the underlying type functions on component collections.
namespace cc_type_tests {
using namespace detail;
// detail::component_collection_contains_tag tests
static_assert(component_collection_contains_tag<cc_fb, foo_tag>::value, "");
static_assert(component_collection_contains_tag<cc_fb, bar_tag>::value, "");
static_assert(!component_collection_contains_tag<cc_fb, foo>::value, "");
static_assert(!component_collection_contains_tag<cc_fb, zap_tag>::value, "");
// detail::component_collection_is_convertible tests
static_assert(component_collection_is_convertible<cc_fb, cc_fb>::value, "");
static_assert(component_collection_is_convertible<cc_b, cc_b>::value, "");
static_assert(component_collection_is_convertible<cc_fbz, cc_fbz>::value, "");
static_assert(component_collection_is_convertible<cc_fb, cc_b>::value, "");
static_assert(!component_collection_is_convertible<cc_b, cc_fb>::value, "");
static_assert(component_collection_is_convertible<cc_fbz, cc_fb>::value, "");
static_assert(!component_collection_is_convertible<cc_fb, cc_fbz>::value, "");
static_assert(component_collection_is_convertible<cc_fbz, cc_b>::value, "");
static_assert(!component_collection_is_convertible<cc_b, cc_fbz>::value, "");
static_assert(component_collection_is_convertible<cc_fzb, cc_fz>::value, "");
static_assert(component_collection_is_convertible<cc_fzb, cc_bz>::value, "");
} // namespace cc_type_tests

TEST_CASE("static component_collection conversions", "[component_collections]")
{
    storage_type storage;
    cc_fb mc_fb(&storage);
    REQUIRE(mc_fb.storage == &storage);
    cc_b mc_b(mc_fb);
    REQUIRE(mc_b.storage == &storage);
}

TEST_CASE("static component access", "[component_collections]")
{
    storage_type storage;
    cc_empty mc_empty(&storage);
    REQUIRE(!has_component<foo_tag>(mc_empty));
    REQUIRE(!has_component<bar_tag>(mc_empty));

    cc_b mc_b = add_component<bar_tag>(mc_empty, bar(1));
    REQUIRE(boost::any_cast<bar>(get_component<bar_tag>(mc_b)).i == 1);
    REQUIRE(!has_component<foo_tag>(mc_b));
    REQUIRE(has_component<bar_tag>(mc_b));

    cc_fb mc_fb = add_component<foo_tag>(mc_b, foo());
    REQUIRE(boost::any_cast<bar>(get_component<bar_tag>(mc_fb)).i == 1);
    REQUIRE(boost::any_cast<foo>(get_component<foo_tag>(mc_fb)).b == false);

    cc_f mc_f = remove_component<bar_tag>(mc_fb);
    REQUIRE(boost::any_cast<foo>(get_component<foo_tag>(mc_f)).b == false);
}

namespace {

using boost::any;
using boost::any_cast;
using std::string;

template<class Tag>
struct component_printer
{
};

template<>
struct component_printer<foo_tag>
{
    static string
    apply(any const& x)
    {
        foo const& f = any_cast<foo>(x);
        return f.b ? "foo: true; " : "foo: false; ";
    }
};

template<>
struct component_printer<bar_tag>
{
    static string
    apply(any const& x)
    {
        bar const& b = any_cast<bar>(x);
        return "bar: " + std::to_string(b.i) + "; ";
    }
};

struct reducer
{
    template<class Tag, class Data>
    string
    operator()(Tag tag, Data data, string z)
    {
        return component_printer<Tag>::apply(data) + z;
    }
};

} // namespace

TEST_CASE("collection folding", "[component_collections]")
{
    storage_type storage;
    cc_empty mc_empty(&storage);
    cc_b mc_b = add_component<bar_tag>(mc_empty, bar(1));
    cc_fb mc_fb = add_component<foo_tag>(mc_b, foo());

    auto reduction = fold_over_components(mc_fb, reducer(), string());
    REQUIRE(reduction == "foo: false; bar: 1; ");
}
