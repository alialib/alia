#include "demo.hpp"

void
sandbox_ui(dom::context ctx)
{
    dom::text(ctx, "It seems to be working!");

    auto flag = get_state(ctx, false);
    dom::button(ctx, "Toggle!", toggle(flag));
    dom::colored_box(
        ctx,
        smooth(ctx, conditional(flag, rgb8(210, 210, 220), rgb8(50, 50, 55))));

    dom::element(ctx, "hr");

    dom::text(ctx, "Enter a number:");
    auto n = get_state(ctx, 0);
    dom::input(ctx, n);
    dom::text(ctx, smooth(ctx, n));
    dom::button(ctx, "Add 100", n += 100);
    dom::button(ctx, "Reset", n <<= 4);
}

void
init_sandbox(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        sandbox_ui(ctx);
    });
}

static demo the_sandbox_demo("sandbox", init_sandbox);
