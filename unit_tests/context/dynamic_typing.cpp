#define ALIA_DYNAMIC_CONTEXT_CHECKS

#include <alia/context/typing.hpp>

#include <alia/context/storage.hpp>

#include <testing.hpp>

using namespace alia::impl;

// This shouldn't have been defined.
#ifdef ALIA_STATIC_CONTEXT_CHECKS
#error ALIA_STATIC_CONTEXT_CHECKS defined
#endif

// Define some arbitrary tag and data types.
struct foo
{
    bool b = false;
};
ALIA_DEFINE_TAGGED_TYPE(foo_tag, foo&)
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
ALIA_DEFINE_TAGGED_TYPE(bar_tag, bar&)
struct zap
{
    double d = 0;
};
ALIA_DEFINE_TAGGED_TYPE(zap_tag, zap&)

// Define some arbitrary collection types.
using storage_type = generic_tagged_storage<any_ref>;
using cc_empty = empty_structural_collection<storage_type>;
using cc_b = add_tagged_data_type_t<cc_empty, bar_tag>;
using cc_fb = add_tagged_data_type_t<cc_b, foo_tag>;
using cc_z = add_tagged_data_type_t<cc_empty, zap_tag>;
using cc_bz = add_tagged_data_type_t<cc_z, bar_tag>;
using cc_fbz = add_tagged_data_types_t<cc_empty, zap_tag, bar_tag, foo_tag>;
using cc_fz = remove_tagged_data_type_t<cc_fbz, bar_tag>;
using cc_f = add_tagged_data_type_t<cc_empty, foo_tag>;
using cc_fzb = merge_structural_collections_t<cc_fz, cc_bz>;

TEST_CASE("dynamic structural_collection conversions", "[context][typing]")
{
    storage_type storage;
    cc_fb mc_fb(&storage);
    REQUIRE(mc_fb.storage == &storage);
    cc_b mc_b(mc_fb);
    REQUIRE(mc_b.storage == &storage);
}

TEST_CASE("dynamic tagged data access", "[context][typing]")
{
    storage_type storage;
    cc_empty mc_empty = make_empty_structural_collection(&storage);
    REQUIRE(!storage.has<bar_tag>());

    bar b(1);
    cc_b mc_b = add_tagged_data<bar_tag>(mc_empty, std::ref(b));
    REQUIRE(storage.has<bar_tag>());
    REQUIRE(has_tagged_data<bar_tag>(mc_b));
    REQUIRE(get_tagged_data<bar_tag>(mc_b).i == 1);
    REQUIRE(!storage.has<foo_tag>());
    REQUIRE(!has_tagged_data<foo_tag>(mc_b));
    REQUIRE_THROWS_AS(
        get_tagged_data<foo_tag>(mc_b), tagged_data_not_found<foo_tag>);

    foo f;
    cc_fb mc_fb = add_tagged_data<foo_tag>(mc_b, std::ref(f));
    REQUIRE(get_tagged_data<bar_tag>(mc_fb).i == 1);
    REQUIRE(get_tagged_data<foo_tag>(mc_fb).b == false);

    cc_f mc_f = remove_tagged_data<bar_tag>(mc_fb);
    REQUIRE(get_tagged_data<foo_tag>(mc_f).b == false);
    REQUIRE_THROWS_AS(
        get_tagged_data<bar_tag>(mc_f), tagged_data_not_found<bar_tag>);
}
