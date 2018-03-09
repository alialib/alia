#include <alia/contexts.hpp>

#include <catch.hpp>

using namespace alia;

struct foo_tag
{
};
struct bar_tag
{
};
struct baz_tag
{
};

struct foo
{
};
struct bar
{
};
struct baz
{
};

using my_context
    = context<context_component<foo_tag, foo>, context_component<bar_tag, bar>>;

using my_context_2 = context<context_component<bar_tag, bar>>;

using my_context_3 = context<
    context_component<foo_tag, foo>,
    context_component<bar_tag, bar>,
    context_component<baz_tag, baz>>;

static_assert(context_contains_tag<my_context, foo_tag>::value, "");
static_assert(context_contains_tag<my_context, bar_tag>::value, "");
static_assert(!context_contains_tag<my_context, foo>::value, "");
static_assert(!context_contains_tag<my_context, baz_tag>::value, "");

static_assert(context_is_convertible<my_context, my_context>::value, "");
static_assert(context_is_convertible<my_context_2, my_context_2>::value, "");
static_assert(context_is_convertible<my_context_3, my_context_3>::value, "");
static_assert(context_is_convertible<my_context, my_context_2>::value, "");
static_assert(!context_is_convertible<my_context_2, my_context>::value, "");
static_assert(context_is_convertible<my_context_3, my_context>::value, "");
static_assert(!context_is_convertible<my_context, my_context_3>::value, "");
static_assert(context_is_convertible<my_context_3, my_context_2>::value, "");
static_assert(!context_is_convertible<my_context_2, my_context_3>::value, "");

TEST_CASE("contexts", "[contexts]")
{
    my_context mc;
    my_context_2 mc2(mc);
}
