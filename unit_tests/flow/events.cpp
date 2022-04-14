#define ALIA_LOWERCASE_MACROS

#include <alia/flow/events.hpp>

#include <testing.hpp>

#include <alia/actions/operators.hpp>
#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>
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

    refresh_handler(
        ctx, [id](auto ctx) { get<my_tag>(ctx).push_back(externalize(id)); });

    event_handler<my_event>(
        ctx, [&](auto, auto& e) { e.visited += read_signal(label) + ";"; });

    targeted_event_handler<my_event>(ctx, id, [&](auto, my_event& event) {
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
        my_context ctx = extend_context<my_tag>(vanilla_ctx, ids);
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

namespace {

struct bad_event
{
};

} // namespace

TEST_CASE("event error propagation", "[flow][events]")
{
    alia::system sys;
    initialize_system(sys, [&](context ctx) {
        event_handler<bad_event>(
            ctx, [&](auto, auto&) { static_cast<void>(std::string().at(0)); });
    });

    refresh_system(sys);

    bad_event event;
    REQUIRE_THROWS_AS(dispatch_event(sys, event), std::out_of_range);

    refresh_system(sys);
}

TEST_CASE("on_init/on_activate", "[flow][events]")
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

TEST_CASE("on_observed_value events", "[flow][events]")
{
    bool has_value = false;
    int value = 0;

    int gain_count = 0;
    int gain_shadow = 0;

    int loss_count = 0;
    int loss_shadow = 0;

    int change_count = 0;
    int change_shadow = 0;

    alia::system sys;
    initialize_system(sys, [&](context ctx) {
        refresh_handler(ctx, [&](auto) {
            gain_shadow = gain_count;
            loss_shadow = loss_count;
            change_shadow = change_count;
        });

        auto signal
            = lambda_reader([&] { return has_value; }, [&] { return value; });

        on_observed_value_gain(ctx, signal, ++direct(gain_count));
        on_observed_value_loss(ctx, signal, ++direct(loss_count));
        on_observed_value_change(ctx, signal, ++direct(change_count));
    });

    refresh_system(sys);
    REQUIRE(gain_count == 0);
    REQUIRE(gain_shadow == 0);
    REQUIRE(loss_count == 0);
    REQUIRE(loss_shadow == 0);
    REQUIRE(change_count == 0);
    REQUIRE(change_shadow == 0);

    refresh_system(sys);
    REQUIRE(gain_count == 0);
    REQUIRE(gain_shadow == 0);
    REQUIRE(loss_count == 0);
    REQUIRE(loss_shadow == 0);
    REQUIRE(change_count == 0);
    REQUIRE(change_shadow == 0);

    has_value = true;
    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 0);
    REQUIRE(loss_shadow == 0);
    REQUIRE(change_count == 1);
    REQUIRE(change_shadow == 1);

    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 0);
    REQUIRE(loss_shadow == 0);
    REQUIRE(change_count == 1);
    REQUIRE(change_shadow == 1);

    value = 2;
    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 0);
    REQUIRE(loss_shadow == 0);
    REQUIRE(change_count == 2);
    REQUIRE(change_shadow == 2);

    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 0);
    REQUIRE(loss_shadow == 0);
    REQUIRE(change_count == 2);
    REQUIRE(change_shadow == 2);

    value = 1;
    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 0);
    REQUIRE(loss_shadow == 0);
    REQUIRE(change_count == 3);
    REQUIRE(change_shadow == 3);

    has_value = false;
    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 1);
    REQUIRE(loss_shadow == 1);
    REQUIRE(change_count == 4);
    REQUIRE(change_shadow == 4);

    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 1);
    REQUIRE(loss_shadow == 1);
    REQUIRE(change_count == 4);
    REQUIRE(change_shadow == 4);

    value = 3;
    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 1);
    REQUIRE(loss_shadow == 1);
    REQUIRE(change_count == 4);
    REQUIRE(change_shadow == 4);
}

TEST_CASE("on_value events", "[flow][events]")
{
    bool has_value = false;
    int value = 0;

    int gain_count = 0;
    int gain_shadow = 0;

    int loss_count = 0;
    int loss_shadow = 0;

    int change_count = 0;
    int change_shadow = 0;

    alia::system sys;
    initialize_system(sys, [&](context ctx) {
        refresh_handler(ctx, [&](auto) {
            gain_shadow = gain_count;
            loss_shadow = loss_count;
            change_shadow = change_count;
        });

        auto signal
            = lambda_reader([&] { return has_value; }, [&] { return value; });

        on_value_gain(ctx, signal, ++direct(gain_count));
        on_value_loss(ctx, signal, ++direct(loss_count));
        on_value_change(ctx, signal, ++direct(change_count));
    });

    refresh_system(sys);
    REQUIRE(gain_count == 0);
    REQUIRE(gain_shadow == 0);
    REQUIRE(loss_count == 1);
    REQUIRE(loss_shadow == 1);
    REQUIRE(change_count == 1);
    REQUIRE(change_shadow == 1);

    refresh_system(sys);
    REQUIRE(gain_count == 0);
    REQUIRE(gain_shadow == 0);
    REQUIRE(loss_count == 1);
    REQUIRE(loss_shadow == 1);
    REQUIRE(change_count == 1);
    REQUIRE(change_shadow == 1);

    has_value = true;
    refresh_system(sys);
    REQUIRE(gain_count == 1);
    REQUIRE(gain_shadow == 1);
    REQUIRE(loss_count == 1);
    REQUIRE(loss_shadow == 1);
    REQUIRE(change_count == 2);
    REQUIRE(change_shadow == 2);
}

TEST_CASE("error isolation", "[flow][events]")
{
    int n = 0;

    alia::system sys;
    initialize_system(sys, [&](context ctx) {
        on_value_change(ctx, value(n), callback([&] {
                            static_cast<void>(std::string("abc").at(n));
                        }));
    });

    int error_count = 0;
    set_error_handler(sys, [&](std::exception_ptr) { ++error_count; });

    refresh_system(sys);
    REQUIRE(error_count == 0);

    n = 2;
    refresh_system(sys);
    REQUIRE(error_count == 0);

    ++n;
    refresh_system(sys);
    REQUIRE(error_count == 1);

    refresh_system(sys);
    REQUIRE(error_count == 1);

    ++n;
    refresh_system(sys);
    REQUIRE(error_count == 2);
}
