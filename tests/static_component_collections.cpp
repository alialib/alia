#include <alia/component_collection.hpp>

#include <catch.hpp>

using namespace alia;

// component_collection.hpp is supposed to define this by default.
#ifndef ALIA_STATIC_COMPONENT_CHECKING
#error ALIA_STATIC_COMPONENT_CHECKING not defined
#endif

// Our test components...
struct foo_tag
{
};
struct foo
{
};
struct bar_tag
{
};
struct bar
{
};
struct zap_tag
{
};
struct zap
{
};

// Define some arbitrary component collections.
using storage_type = generic_component_storage<any>;
using cc_empty = empty_component_collection<storage_type>;
using cc_b = add_component_type_t<cc_empty, bar_tag, bar>;
using cc_fb = add_component_type_t<cc_b, foo_tag, foo>;
using cc_z = add_component_type_t<cc_empty, zap_tag, zap>;
using cc_bz = add_component_type_t<cc_z, bar_tag, bar>;
using cc_fbz = add_component_type_t<cc_bz, foo_tag, foo>;

static_assert(
    detail::component_collection_contains_tag<cc_fb, foo_tag>::value, "");
static_assert(
    detail::component_collection_contains_tag<cc_fb, bar_tag>::value, "");
static_assert(
    !detail::component_collection_contains_tag<cc_fb, foo>::value, "");
static_assert(
    !detail::component_collection_contains_tag<cc_fb, zap_tag>::value, "");

static_assert(
    detail::component_collection_is_convertible<cc_fb, cc_fb>::value, "");
static_assert(
    detail::component_collection_is_convertible<cc_b, cc_b>::value, "");
static_assert(
    detail::component_collection_is_convertible<cc_fbz, cc_fbz>::value, "");
static_assert(
    detail::component_collection_is_convertible<cc_fb, cc_b>::value, "");
static_assert(
    !detail::component_collection_is_convertible<cc_b, cc_fb>::value, "");
static_assert(
    detail::component_collection_is_convertible<cc_fbz, cc_fb>::value, "");
static_assert(
    !detail::component_collection_is_convertible<cc_fb, cc_fbz>::value, "");
static_assert(
    detail::component_collection_is_convertible<cc_fbz, cc_b>::value, "");
static_assert(
    !detail::component_collection_is_convertible<cc_b, cc_fbz>::value, "");

TEST_CASE("component_collections", "[component_collections]")
{
    storage_type storage;
    cc_fb mc_fb(&storage);
    REQUIRE(mc_fb.storage == &storage);
    cc_b mc_b(mc_fb);
    REQUIRE(mc_b.storage == &storage);
}
