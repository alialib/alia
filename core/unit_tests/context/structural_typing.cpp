#include <alia/core/context/structural_typing.hpp>

#include <alia/core/context/storage.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace alia::detail;

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

// Test the underlying mechanics of adding and removing tags in lists.
namespace list_tests {
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

// Define some arbitrary collection types.
using storage_type = generic_tagged_storage<std::any>;
using cc_empty = empty_structural_collection<storage_type>;
using cc_b = add_tagged_data_type_t<cc_empty, bar_tag>;
using cc_fb = add_tagged_data_type_t<cc_b, foo_tag>;
using cc_z = add_tagged_data_type_t<cc_empty, zap_tag>;
using cc_bz = add_tagged_data_type_t<cc_z, bar_tag>;
using cc_fbz = add_tagged_data_types_t<cc_empty, zap_tag, bar_tag, foo_tag>;
using cc_fz = remove_tagged_data_type_t<cc_fbz, bar_tag>;
using cc_f = add_tagged_data_type_t<cc_empty, foo_tag>;
using cc_fzb = merge_structural_collections_t<cc_fz, cc_bz>;

// Test the underlying type functions on collections.
namespace cc_type_tests {
// structural_collection_contains_tag tests
static_assert(structural_collection_contains_tag<cc_fb, foo_tag>::value, "");
static_assert(structural_collection_contains_tag<cc_fb, bar_tag>::value, "");
static_assert(!structural_collection_contains_tag<cc_fb, foo>::value, "");
static_assert(!structural_collection_contains_tag<cc_fb, zap_tag>::value, "");
// structural_collection_is_convertible tests
static_assert(structural_collection_is_convertible<cc_fb, cc_fb>::value, "");
static_assert(structural_collection_is_convertible<cc_b, cc_b>::value, "");
static_assert(structural_collection_is_convertible<cc_fbz, cc_fbz>::value, "");
static_assert(structural_collection_is_convertible<cc_fb, cc_b>::value, "");
static_assert(!structural_collection_is_convertible<cc_b, cc_fb>::value, "");
static_assert(structural_collection_is_convertible<cc_fbz, cc_fb>::value, "");
static_assert(!structural_collection_is_convertible<cc_fb, cc_fbz>::value, "");
static_assert(structural_collection_is_convertible<cc_fbz, cc_b>::value, "");
static_assert(!structural_collection_is_convertible<cc_b, cc_fbz>::value, "");
static_assert(structural_collection_is_convertible<cc_fzb, cc_fz>::value, "");
static_assert(structural_collection_is_convertible<cc_fzb, cc_bz>::value, "");
} // namespace cc_type_tests

TEST_CASE("static structural_collection conversions", "[context][typing]")
{
    storage_type storage;
    cc_fb mc_fb(&storage);
    REQUIRE(mc_fb.storage == &storage);
    cc_b mc_b(mc_fb);
    REQUIRE(mc_b.storage == &storage);
}

TEST_CASE("static structural_collection access", "[context][typing]")
{
    storage_type storage;
    cc_empty mc_empty = make_empty_structural_collection(&storage);
    REQUIRE(!has_tagged_data<foo_tag>(mc_empty));
    REQUIRE(!has_tagged_data<bar_tag>(mc_empty));

    bar b(1);
    cc_b mc_b = add_tagged_data<bar_tag>(mc_empty, std::ref(b));
    REQUIRE(get_tagged_data<bar_tag>(mc_b).i == 1);
    REQUIRE(!has_tagged_data<foo_tag>(mc_b));
    REQUIRE(has_tagged_data<bar_tag>(mc_b));

    foo f;
    cc_fb mc_fb = add_tagged_data<foo_tag>(mc_b, std::ref(f));
    REQUIRE(get_tagged_data<bar_tag>(mc_fb).i == 1);
    REQUIRE(get_tagged_data<foo_tag>(mc_fb).b == false);

    cc_f mc_f = remove_tagged_data<bar_tag>(mc_fb);
    REQUIRE(get_tagged_data<foo_tag>(mc_f).b == false);
    // It's OK to remove a tag that's not in the collection.
    cc_f mc_f2 = remove_tagged_data<bar_tag>(mc_f);
    REQUIRE(get_tagged_data<foo_tag>(mc_f2).b == false);
}

ALIA_DEFINE_TAGGED_TYPE(int_tag, int)

TEST_CASE("tagged data casting", "[context][typing]")
{
    generic_tagged_storage<int> storage;
    auto empty = make_empty_structural_collection(&storage);
    auto ctx = add_tagged_data<int_tag>(empty, 1);
    REQUIRE(get_tagged_data<int_tag>(ctx) == 1);
}

namespace {

using std::string;

template<class Tag>
struct tagged_data_printer
{
};

template<>
struct tagged_data_printer<foo_tag>
{
    static string
    apply(foo const& f)
    {
        return f.b ? "foo: true; " : "foo: false; ";
    }
};

template<>
struct tagged_data_printer<bar_tag>
{
    static string
    apply(bar const& b)
    {
        return "bar: " + std::to_string(b.i) + "; ";
    }
};

struct reducer
{
    template<class Tag, class Data>
    string
    operator()(Tag, Data data, string z)
    {
        return tagged_data_printer<Tag>::apply(data) + z;
    }
};

} // namespace

TEST_CASE("collection folding", "[context][typing]")
{
    storage_type storage;
    auto mc_empty = make_empty_structural_collection(&storage);
    bar b(1);
    auto mc_b = add_tagged_data<bar_tag>(mc_empty, std::ref(b));
    foo f;
    auto mc_fb = add_tagged_data<foo_tag>(mc_b, std::ref(f));

    auto reduction = alia::fold_over_collection(mc_fb, reducer(), string());
    REQUIRE(reduction == "foo: false; bar: 1; ");
}
