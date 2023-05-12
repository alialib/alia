#include "demo.hpp"

#include <random>

namespace pure_component {

static int player_ui_invocation_count = 0;

// clang-format off
/// [pure-component]
// This will be our pure component.
void
player_ui(html::context ctx, duplex<std::string> name)
{
    // Don't do this! This is just to illustrate when this component is
    // actually invoked.
    ++player_ui_invocation_count;

    // Pure components can have internal state.
    auto score = get_state(ctx, 0);

    // Here's the UI defined by our pure component.
    div(ctx, "item", [&] {
        h4(ctx, name);
        p(ctx, printf(ctx, "%d points", score));
        button(ctx, "GOAL!", ++score);
    });
}

void
main_ui(html::context ctx)
{
    // Get some state to pass into our pure component.
    auto name = get_state(ctx, "Name");

    // Allow the player name to be edited outside the pure component.
    html::input(ctx, name);

    hr(ctx);

    // Invoke the pure component.
    // player_ui will only actually be invoked if `name` changes or if there's
    // an event inside it.
    invoke_pure_component(ctx, player_ui, name);

    // For illustration purposes...
    html::p(ctx,
        printf(ctx,
            "player_ui() has been invoked %i time(s).",
            player_ui_invocation_count));

    hr(ctx);

    // Here's some other UI to demonstrate that we can interact with it without
    // invoking player_ui.
    auto n = get_state(ctx, 0);
    html::p(ctx, printf(ctx, "N is %i.", n));
    html::button(ctx, "Increment", ++n);
}
/// [pure-component]
// clang-format on

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { main_ui(ctx); });
}

static demo the_demo("pure-component", init_demo);

} // namespace pure_component
