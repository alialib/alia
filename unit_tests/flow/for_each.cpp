#define ALIA_LOWERCASE_MACROS

#include <alia/flow/for_each.hpp>

#include <algorithm>
#include <sstream>

#include <alia/flow/events.hpp>
#include <alia/signals/adaptors.hpp>
#include <alia/signals/application.hpp>
#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>
#include <alia/signals/operators.hpp>
#include <alia/system.hpp>

#include <catch.hpp>

using namespace alia;

using std::string;

namespace {

struct ostream_event
{
    std::ostream* stream;
};

void
do_ostream_text(context ctx, input<string> const& text)
{
    ostream_event* oe;
    if (detect_event(ctx, &oe) && signal_is_readable(text))
    {
        *oe->stream << read_signal(text) << ";";
    }
}

// Define a simple custom structure to represent the 'items' we'll collect.

struct my_item
{
    string id;
};

bool
operator==(my_item a, my_item b)
{
    return a.id == b.id;
}

bool
operator<(my_item a, my_item b)
{
    return a.id < b.id;
}

auto
get_alia_id(my_item const& item)
{
    return make_id(item.id);
}

template<class Controller>
void
check_traversal(
    alia::system& sys,
    Controller const& controller,
    string const& expected_output)
{
    sys.controller = controller;
    {
        ostream_event oe;
        std::ostringstream s;
        oe.stream = &s;
        dispatch_event(sys, oe);
        REQUIRE(s.str() == expected_output);
    }
}

} // namespace

TEST_CASE("simple string vector", "[for_each][vector]")
{
    alia::system sys;

    std::vector<string> container{"foo", "bar", "baz"};

    auto controller = [&](context ctx) {
        for_each(ctx, direct(container), do_ostream_text);
    };

    check_traversal(sys, controller, "foo;bar;baz;");
}

TEST_CASE("simple item vector", "[for_each][vector]")
{
    alia::system sys;

    std::vector<my_item> container{{"apple"}, {"banana"}, {"cherry"}};

    auto controller = [&](context ctx) {
        for_each(
            ctx,
            direct(container),
            [](context ctx, input<my_item> const& item) {
                do_ostream_text(ctx, alia_field(item, id));
            });
    };

    check_traversal(sys, controller, "apple;banana;cherry;");
}

TEST_CASE("string vector reordering", "[for_each][vector]")
{
    alia::system sys;

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::vector<string> container{"foo", "bar", "baz"};

    auto controller = [&](context ctx) {
        for_each(
            ctx,
            direct(container),
            [&](context ctx, input<string> const& item) {
                do_ostream_text(
                    ctx, apply(ctx, counting_identity, simplify_id(item)));
            });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no additional
    // calls are made.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since two items switched places, two additional calls were made.
    check_traversal(sys, controller, "baz;bar;foo;");
    REQUIRE(call_count == 5);
}

TEST_CASE("item vector reordering", "[for_each][vector]")
{
    alia::system sys;

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::vector<my_item> container{{"apple"}, {"banana"}, {"cherry"}};

    auto controller = [&](context ctx) {
        for_each(
            ctx,
            direct(container),
            [&](context ctx, input<my_item> const& item) {
                do_ostream_text(
                    ctx,
                    apply(
                        ctx,
                        counting_identity,
                        simplify_id(alia_field(item, id))));
            });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "apple;banana;cherry;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no additional
    // calls are made.
    check_traversal(sys, controller, "apple;banana;cherry;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since my_item defines get_alia_id(), the graph data properly follows
    // the items around, so there are no additional calls.
    check_traversal(sys, controller, "cherry;banana;apple;");
    REQUIRE(call_count == 3);
}
