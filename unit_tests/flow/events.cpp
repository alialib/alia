#define ALIA_LOWERCASE_MACROS

#include <alia/flow/events.hpp>

#include <testing.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/operators.hpp>
#include <alia/signals/state.hpp>
#include <alia/system/internals.hpp>

using namespace alia;
using std::string;

namespace {

struct my_event : targeted_event
{
    string visited;
    string result;
};

ALIA_DEFINE_TAGGED_TYPE(my_tag, std::vector<external_component_id>&)

typedef extend_context_type_t<context, my_tag> my_context;

static void
do_my_thing(my_context ctx, readable<string> label)
{
    auto id = get_component_id(ctx);

    on_refresh(ctx, [id](auto ctx) {
        ctx.template get<my_tag>().push_back(externalize(id));
    });

    on_event<my_event>(
        ctx, [&](auto, auto& e) { e.visited += read_signal(label) + ";"; });

    on_targeted_event<my_event>(ctx, id, [&](auto, my_event& event) {
        event.result = read_signal(label);
    });
}

} // namespace

TEST_CASE("node IDs", "[flow][events]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    REQUIRE(!is_valid(null_component_id));

    std::vector<external_component_id> ids;

    sys.controller = [&](context vanilla_ctx) {
        my_context ctx = vanilla_ctx.add<my_tag>(ids);
        do_my_thing(ctx, value("one"));
        do_my_thing(ctx, value("two"));
    };
    refresh_system(sys);

    REQUIRE(ids.size() == 2);
    REQUIRE(is_valid(ids[0]));
    REQUIRE(is_valid(ids[1]));

    {
        my_event event;
        dispatch_targeted_event(sys, event, ids[0]);
        REQUIRE(event.visited == "one;");
        REQUIRE(event.result == "one");
    }
    {
        my_event event;
        dispatch_targeted_event(sys, event, ids[1]);
        REQUIRE(event.visited == "one;two;");
        REQUIRE(event.result == "two");
    }
}

TEST_CASE("on_init/on_activate", "[signals][state]")
{
    bool active = false;

    int init_count = 0;
    int activate_count = 0;

    alia::system sys;
    initialize_system(sys, [&](context ctx) {
        ALIA_IF(active)
        {
            on_init(ctx, ++direct(init_count));
            on_activate(ctx, ++direct(activate_count));
        }
        ALIA_END
    });

    active = false;
    refresh_system(sys);
    REQUIRE(init_count == 0);
    REQUIRE(activate_count == 0);

    active = true;
    refresh_system(sys);
    REQUIRE(init_count == 1);
    REQUIRE(activate_count == 1);

    active = true;
    refresh_system(sys);
    REQUIRE(init_count == 1);
    REQUIRE(activate_count == 1);

    active = false;
    refresh_system(sys);
    REQUIRE(init_count == 1);
    REQUIRE(activate_count == 1);

    active = true;
    refresh_system(sys);
    REQUIRE(init_count == 1);
    REQUIRE(activate_count == 2);

    active = true;
    refresh_system(sys);
    REQUIRE(init_count == 1);
    REQUIRE(activate_count == 2);

    active = false;
    refresh_system(sys);
    REQUIRE(init_count == 1);
    REQUIRE(activate_count == 2);

    active = true;
    refresh_system(sys);
    REQUIRE(init_count == 1);
    REQUIRE(activate_count == 3);
}
