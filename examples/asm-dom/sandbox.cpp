#include "demo.hpp"

void
do_sandbox_ui(dom::context ctx)
{
    dom::do_text(ctx, "It seems to be working!");

    auto flag = get_state(ctx, false);
    dom::do_button(ctx, "Toggle!", toggle(flag));
    dom::do_colored_box(
        ctx,
        smooth(ctx, conditional(flag, rgb8(210, 210, 220), rgb8(50, 50, 55))));

    do_hr(ctx);

    dom::do_text(ctx, "Enter a number:");
    auto n = get_state(ctx, 0);
    do_input(ctx, n);
    do_text(ctx, smooth(ctx, n));
    do_button(ctx, "Add 100", n += 100);
    do_button(ctx, "Reset", n <<= 4);
}

void
init_sandbox(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_sandbox_ui(ctx);
    });
}

static demo the_sandbox_demo("sandbox", init_sandbox);
