#define ALIA_LOWERCASE_MACROS

#include <alia/core/flow/for_each.hpp>

#include <algorithm>
#include <list>
#include <map>

#include <alia/core/signals/adaptors.hpp>
#include <alia/core/signals/application.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/lambdas.hpp>
#include <alia/core/signals/operators.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "traversal.hpp"

using namespace alia;

using std::string;

namespace {

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
get_alia_item_id(my_item const& item)
{
    return make_id(item.id);
}

} // namespace

TEST_CASE("string vector", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::vector<string> container{"foo", "bar", "baz"};

    auto controller = [&](context ctx) {
        for_each(ctx, direct(container), [&](readable<string> const& item) {
            do_text(ctx, apply(ctx, counting_identity, simplify_id(item)));
        });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since two items switched places, two additional calls were made.
    check_traversal(sys, controller, "baz;bar;foo;");
    REQUIRE(call_count == 5);
}

TEST_CASE("raw string vector", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::vector<string> container{"foo", "bar", "baz"};

    auto controller = [&](context ctx) {
        for_each(ctx, container, [&](string& item) {
            do_text(ctx, apply(ctx, counting_identity, direct(item)));
        });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since two items switched places, two additional calls were made.
    check_traversal(sys, controller, "baz;bar;foo;");
    REQUIRE(call_count == 5);
}

TEST_CASE("for_each over vector of string signals", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::vector<string_literal_signal> container{"foo", "bar", "baz"};

    auto controller = [&](context ctx) {
        for_each(ctx, container, [&](auto item) {
            do_text(ctx, apply(ctx, counting_identity, item));
        });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since two items switched places, two additional calls were made.
    check_traversal(sys, controller, "baz;bar;foo;");
    REQUIRE(call_count == 5);
}

TEST_CASE("string vector with naming", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

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
            [&](naming_context& nc, readable<string> const& item) {
                named_block nb(nc, make_id(read_signal(item)));
                do_text(ctx, apply(ctx, counting_identity, simplify_id(item)));
            });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since we're doing our own custom naming, there should be no additional
    // calls even after items move around.
    check_traversal(sys, controller, "baz;bar;foo;");
    REQUIRE(call_count == 3);
}

TEST_CASE("string vector with index", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

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
            [&](size_t index, readable<string> const& item) {
                do_text(ctx, value(index));
                do_text(ctx, apply(ctx, counting_identity, simplify_id(item)));
            });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "0;foo;1;bar;2;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "0;foo;1;bar;2;baz;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since two items switched places, two additional calls were made.
    check_traversal(sys, controller, "0;baz;1;bar;2;foo;");
    REQUIRE(call_count == 5);
}

TEST_CASE("string vector with naming and index", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

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
            [&](naming_context& nc,
                size_t index,
                readable<string> const& item) {
                named_block nb(nc, make_id(read_signal(item)));
                do_text(ctx, value(index));
                do_text(ctx, apply(ctx, counting_identity, simplify_id(item)));
            });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "0;foo;1;bar;2;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "0;foo;1;bar;2;baz;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since we're doing our own custom naming, there should be no additional
    // calls even after items move around.
    check_traversal(sys, controller, "0;baz;1;bar;2;foo;");
    REQUIRE(call_count == 3);
}

TEST_CASE("item vector", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::vector<my_item> container{{"apple"}, {"banana"}, {"cherry"}};

    auto controller = [&](context ctx) {
        for_each(ctx, direct(container), [&](readable<my_item> const& item) {
            do_text(
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

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "apple;banana;cherry;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since my_item defines get_alia_item_id(), the graph data properly
    // follows the items around, so there are no additional calls.
    check_traversal(sys, controller, "cherry;banana;apple;");
    REQUIRE(call_count == 3);
}

TEST_CASE("simple map", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::map<string, int> container{{"foo", 2}, {"bar", 0}, {"baz", 3}};

    auto controller = [&](context ctx) {
        for_each(
            ctx,
            direct(container),
            [&](readable<string> key, duplex<int> value) {
                do_text(ctx, apply(ctx, counting_identity, simplify_id(key)));
                do_text(ctx, apply(ctx, alia_lambdify(std::to_string), value));
            });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "bar;0;baz;3;foo;2;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "bar;0;baz;3;foo;2;");
    REQUIRE(call_count == 3);

    container["alpha"] = 1;

    // Since map items don't actually move around, the graph data properly
    // follows the items, so the only additional call is for the new item.
    check_traversal(sys, controller, "alpha;1;bar;0;baz;3;foo;2;");
    REQUIRE(call_count == 4);
}

TEST_CASE("item map", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::map<my_item, int> container{{{"foo"}, 2}, {{"bar"}, 0}, {{"baz"}, 3}};

    auto controller = [&](context ctx) {
        for_each(
            ctx,
            direct(container),
            [&](naming_context& nc, readable<my_item> key, duplex<int> value) {
                named_block nb(nc, make_id(read_signal(key)));
                do_text(
                    ctx,
                    apply(
                        ctx,
                        counting_identity,
                        simplify_id(alia_field(key, id))));
                do_text(ctx, apply(ctx, alia_lambdify(std::to_string), value));
            });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "bar;0;baz;3;foo;2;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "bar;0;baz;3;foo;2;");
    REQUIRE(call_count == 3);

    my_item alpha{"alpha"};
    container[alpha] = 1;

    // Since we provided a custom ID, the graph data properly follows the
    // items, so the only additional call is for the new item.
    check_traversal(sys, controller, "alpha;1;bar;0;baz;3;foo;2;");
    REQUIRE(call_count == 4);
}

TEST_CASE("string list", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::list<string> container{"foo", "bar", "baz"};

    auto controller = [&](context ctx) {
        for_each(ctx, direct(container), [&](readable<string> const& item) {
            do_text(ctx, apply(ctx, counting_identity, simplify_id(item)));
        });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    container.reverse();

    // Since list items don't actually move around, the graph data properly
    // follows the items, so there are no additional calls.
    check_traversal(sys, controller, "baz;bar;foo;");
    REQUIRE(call_count == 3);
}

TEST_CASE("unsimplified string list", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::list<string> container{"foo", "bar", "baz"};

    auto controller = [&](context ctx) {
        for_each(ctx, direct(container), [&](readable<string> const& item) {
            do_text(ctx, apply(ctx, counting_identity, item));
        });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "foo;bar;baz;");
    REQUIRE(call_count == 3);

    container.reverse();

    // Since there's no call to simplify_id in the controller, the IDs of the
    // items actually changed, which triggered additional calls.
    check_traversal(sys, controller, "baz;bar;foo;");
    REQUIRE(call_count == 6);
}

TEST_CASE("writing string list items", "[flow][for_each]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    std::list<string> container{"foo", "bar", "baz"};

    auto controller = [&](context ctx) {
        for_each(ctx, direct(container), [&](duplex<string> const& item) {
            write_signal(item, "boo");
        });
    };

    check_traversal(sys, controller, "");
    REQUIRE(container == (std::list<string>{"boo", "boo", "boo"}));
}

TEST_CASE("item list", "[for_each][list]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::list<my_item> container{{"apple"}, {"banana"}, {"cherry"}};

    auto controller = [&](context ctx) {
        for_each(ctx, direct(container), [&](readable<my_item> item) {
            do_text(
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

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "apple;banana;cherry;");
    REQUIRE(call_count == 3);

#ifdef NDEBUG
    // Also do some benchmarking while we're here.
    BENCHMARK("stable list for_each")
    {
        refresh_system(sys);
    };
#endif

    std::reverse(container.begin(), container.end());

    // Since my_item defines get_alia_item_id(), the graph data properly
    // follows the items around, so there are no additional calls.
    check_traversal(sys, controller, "cherry;banana;apple;");
    REQUIRE(call_count == 3);
}

TEST_CASE("for_each over list of item signals", "[for_each][list]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::list<value_signal<my_item>> container{
        value(my_item{"apple"}),
        value(my_item{"banana"}),
        value(my_item{"cherry"})};

    auto controller = [&](context ctx) {
        for_each(
            ctx, container, [&](naming_context& nc, readable<my_item> item) {
                named_block nb(nc, make_id(read_signal(item).id));
                do_text(
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

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "apple;banana;cherry;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since we provided a custom naming scheme, the graph data properly
    // follows the items around, so there are no additional calls.
    check_traversal(sys, controller, "cherry;banana;apple;");
    REQUIRE(call_count == 3);
}

TEST_CASE("for_each over a list of raw items", "[for_each][list]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int call_count = 0;
    auto counting_identity = [&](string s) {
        ++call_count;
        return s;
    };

    std::list<my_item> container{{"apple"}, {"banana"}, {"cherry"}};

    auto controller = [&](context ctx) {
        for_each(ctx, container, [&](my_item& item) {
            do_text(ctx, apply(ctx, counting_identity, value(item.id)));
        });
    };

    // The first time the traversal is done, there is one initial call for each
    // item.
    check_traversal(sys, controller, "apple;banana;cherry;");
    REQUIRE(call_count == 3);

    // For sanity, check that when we reinvoke the same traversal, no
    // additional calls are made.
    check_traversal(sys, controller, "apple;banana;cherry;");
    REQUIRE(call_count == 3);

    std::reverse(container.begin(), container.end());

    // Since my_item defines get_alia_item_id(), the graph data properly
    // follows the items around, so there are no additional calls.
    check_traversal(sys, controller, "cherry;banana;apple;");
    REQUIRE(call_count == 3);
}
