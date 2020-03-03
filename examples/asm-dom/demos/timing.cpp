#include "demo.hpp"

namespace time_signal {

void
do_ui(dom::context ctx)
{
    // clang-format off
/// [time-signal]
dom::do_text(ctx,
    printf(ctx,
        "It's been %d seconds since you started looking at this page.",
        get_animation_tick_count(ctx) / 1000));
/// [time-signal]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(
        the_dom, the_system, dom_id, [](dom::context ctx) { do_ui(ctx); });
}

static demo the_demo("time-signal", init_demo);

} // namespace time_signal

namespace number_smoothing {

void
do_ui(dom::context ctx, bidirectional<int> n)
{
    // clang-format off
/// [number-smoothing]
dom::do_text(ctx, "Enter N:");
dom::do_input(ctx, n);
dom::do_button(ctx, "1", n <<= 1);
dom::do_button(ctx, "100", n <<= 100);
dom::do_button(ctx, "10000", n <<= 10000);
dom::do_text(ctx, "Here's a smoothed view of N:");
dom::do_heading(ctx, "h4", smooth(ctx, n));
/// [number-smoothing]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, 42));
    });
}

static demo the_demo("number-smoothing", init_demo);

} // namespace number_smoothing

namespace color_smoothing {

void
do_ui(dom::context ctx, bidirectional<rgb8> color)
{
    // clang-format off
/// [color-smoothing]
dom::do_colored_box(ctx, smooth(ctx, color));
dom::do_button(ctx, "Go Light", color <<= rgb8(210, 210, 220));
dom::do_button(ctx, "Go Dark", color <<= rgb8(50, 50, 55));
/// [color-smoothing]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, rgb8(50, 50, 55)));
    });
}

static demo the_demo("color-smoothing", init_demo);

} // namespace color_smoothing
