#include "demo.hpp"
#include "emscripten.hpp"

#include <cmath>
#include <future>

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

namespace simple_animation {

void
do_ui(dom::context ctx)
{
    // clang-format off
    /// [simple-animation]
dom::do_colored_box(
    ctx,
    interpolate(
        rgb8(255, 255, 255),
        rgb8(0, 118, 255),
        (1 + std::sin(get_raw_animation_tick_count(ctx) / 600.)) / 2));
    /// [simple-animation]
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

static demo the_demo("simple-animation", init_demo);

} // namespace simple_animation

namespace number_smoothing {

void
do_ui(dom::context ctx, duplex<int> n)
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
do_ui(dom::context ctx, duplex<rgb8> color)
{
    // clang-format off
/// [color-smoothing]
dom::do_colored_box(
    ctx, smooth(ctx, color, animated_transition{ease_in_out_curve, 200}));
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

namespace flickering {

void
do_ui(dom::context ctx, duplex<int> n)
{
    // clang-format off
/// [flickering]
dom::do_text(ctx, "Enter N:");
dom::do_input(ctx, n);
dom::do_button(ctx, "++", ++n);
dom::do_text(ctx, "Here's a flickering signal carrying N * 2:");
dom::do_text(ctx,
    alia::async<int>(ctx,
        [](auto ctx, auto report_result, auto n) {
            web::async_call([=]() {
                report_result(n * 2);
            }, 200);
        },
        n));
/// [flickering]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, 1));
    });
}

static demo the_demo("flickering-demo", init_demo);

} // namespace flickering

namespace deflickering {

void
do_ui(dom::context ctx, duplex<int> n)
{
    // clang-format off
/// [deflickering]
dom::do_text(ctx, "Enter N:");
dom::do_input(ctx, n);
dom::do_button(ctx, "++", ++n);
dom::do_text(ctx, "Here's the same signal from above with deflickering:");
dom::do_text(ctx,
    deflicker(ctx,
        alia::async<int>(ctx,
            [](auto ctx, auto report_result, auto n) {
                web::async_call([=]() {
                    report_result(n * 2);
                }, 200);
            },
            n)));
/// [deflickering]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, 1));
    });
}

static demo the_demo("deflickering-demo", init_demo);

} // namespace deflickering
