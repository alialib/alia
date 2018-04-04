#define ALIA_DYNAMIC_COMPONENT_CHECKING

#include <alia/component_collection.hpp>

#include <catch.hpp>

using namespace alia;

// This shouldn't have been defiend.
#ifdef ALIA_STATIC_COMPONENT_CHECKING
#error ALIA_STATIC_COMPONENT_CHECKING defined
#endif

// Define some arbitrary tag and data types.
struct foo_tag
{
};
struct foo
{
    bool b = false;
};
struct bar_tag
{
};
struct bar
{
    int i = 0;
};
struct zap_tag
{
};
struct zap
{
    double d = 0;
};

// Define some arbitrary component collection types.
using storage_type = generic_component_storage<any>;
using cc_empty = empty_component_collection<storage_type>;
using cc_b = add_component_type_t<cc_empty, bar_tag, bar>;
using cc_fb = add_component_type_t<cc_b, foo_tag, foo>;
using cc_z = add_component_type_t<cc_empty, zap_tag, zap>;
using cc_bz = add_component_type_t<cc_z, bar_tag, bar>;
using cc_fbz = add_component_type_t<cc_bz, foo_tag, foo>;
using cc_fz = remove_component_type_t<cc_fbz, bar_tag>;
using cc_f = add_component_type_t<cc_empty, foo_tag, foo>;
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
    storage_type storage_empty;
    cc_empty mc_empty(&storage_empty);

    storage_type storage_b;
    cc_b mc_b = add_component<bar_tag>(&storage_b, mc_empty, bar{1});
    REQUIRE(any_cast<bar>(get_component<bar_tag>(mc_b))->i == 1);
    REQUIRE_THROWS(get_component<foo_tag>(mc_b));

    storage_type storage_fb;
    cc_fb mc_fb = add_component<foo_tag>(&storage_fb, mc_b, foo());
    REQUIRE(any_cast<bar>(get_component<bar_tag>(mc_fb))->i == 1);
    REQUIRE(any_cast<foo>(get_component<foo_tag>(mc_fb))->b == false);

    storage_type storage_f;
    cc_f mc_f = remove_component<bar_tag>(&storage_f, mc_fb);
    REQUIRE(any_cast<foo>(get_component<foo_tag>(mc_f))->b == false);
    REQUIRE_THROWS(get_component<bar_tag>(mc_f));
}
