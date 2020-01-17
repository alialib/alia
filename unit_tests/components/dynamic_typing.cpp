#define ALIA_DYNAMIC_COMPONENT_CHECKING

#include <alia/components/typing.hpp>

#include <catch.hpp>

#include <boost/any.hpp>

using namespace alia;

// This shouldn't have been defined.
#ifdef ALIA_STATIC_COMPONENT_CHECKING
#error ALIA_STATIC_COMPONENT_CHECKING defined
#endif

// Define some arbitrary tag and data types.
struct foo
{
    bool b = false;
};
ALIA_DEFINE_COMPONENT_TYPE(foo_tag, foo)
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
ALIA_DEFINE_COMPONENT_TYPE(bar_tag, bar)
struct zap
{
    double d = 0;
};
ALIA_DEFINE_COMPONENT_TYPE(zap_tag, zap)

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

TEST_CASE("dynamic component_collection conversions", "[component_collections]")
{
    storage_type storage;
    cc_fb mc_fb(&storage);
    REQUIRE(mc_fb.storage == &storage);
    cc_b mc_b(mc_fb);
    REQUIRE(mc_b.storage == &storage);
}

TEST_CASE("dynamic component access", "[component_collections]")
{
    storage_type storage;
    cc_empty mc_empty(&storage);
    REQUIRE(!has_storage_component<bar_tag>(storage));

    cc_b mc_b = add_component<bar_tag>(mc_empty, bar(1));
    REQUIRE(has_storage_component<bar_tag>(storage));
    REQUIRE(has_component<bar_tag>(mc_b));
    REQUIRE(boost::any_cast<bar>(get_component<bar_tag>(mc_b)).i == 1);
    REQUIRE(!has_storage_component<foo_tag>(storage));
    REQUIRE(!has_component<foo_tag>(mc_b));
    REQUIRE_THROWS_AS(
        get_component<foo_tag>(mc_b), component_not_found<foo_tag>);

    cc_fb mc_fb = add_component<foo_tag>(mc_b, foo());
    REQUIRE(boost::any_cast<bar>(get_component<bar_tag>(mc_fb)).i == 1);
    REQUIRE(boost::any_cast<foo>(get_component<foo_tag>(mc_fb)).b == false);

    cc_f mc_f = remove_component<bar_tag>(mc_fb);
    REQUIRE(boost::any_cast<foo>(get_component<foo_tag>(mc_f)).b == false);
    REQUIRE_THROWS_AS(
        get_component<bar_tag>(mc_f), component_not_found<bar_tag>);
}

namespace {

using boost::any;
using boost::any_cast;
using std::string;

string
print_component(any const& x)
{
    foo const* f = any_cast<foo>(&x);
    if (f)
        return f->b ? "foo: true; " : "foo: false; ";

    bar const* b = any_cast<bar>(&x);
    if (b)
        return "bar: " + std::to_string(b->i) + "; ";

    return "";
}

struct reducer
{
    template<class Data>
    string
    operator()(Data data, string z)
    {
        return print_component(data) + z;
    }
};

} // namespace

TEST_CASE("dynamic collection folding", "[component_collections]")
{
    storage_type storage;
    cc_empty mc_empty(&storage);
    cc_b mc_b = add_component<bar_tag>(mc_empty, bar(1));
    cc_fb mc_fb = add_component<foo_tag>(mc_b, foo());

    auto reduction = fold_over_components(mc_fb, reducer(), string());
    REQUIRE(
        (reduction == "foo: false; bar: 1; "
         || reduction == "bar: 1; foo: false; "));
}
