#include "demo.hpp"
#include "emscripten.hpp"

#include <cmath>
#include <future>

namespace time_signal {

void
demo_ui(dom::context ctx)
{
    // clang-format off
/// [time-signal]
dom::text(ctx,
    alia::printf(ctx,
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
        the_dom, the_system, dom_id, [](dom::context ctx) { demo_ui(ctx); });
}

static demo the_demo("time-signal", init_demo);

} // namespace time_signal

namespace simple_animation {

void
demo_ui(dom::context ctx)
{
    // clang-format off
    /// [simple-animation]
dom::colored_box(
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
        the_dom, the_system, dom_id, [](dom::context ctx) { demo_ui(ctx); });
}

static demo the_demo("simple-animation", init_demo);

} // namespace simple_animation

namespace number_smoothing {

void
demo_ui(dom::context ctx, duplex<int> n)
{
    // clang-format off
/// [number-smoothing]
dom::text(ctx, "Enter N:");
dom::input(ctx, n);
dom::button(ctx, "1", n <<= 1);
dom::button(ctx, "100", n <<= 100);
dom::button(ctx, "10000", n <<= 10000);
dom::text(ctx, "Here's a smoothed view of N:");
dom::text(ctx, smooth(ctx, n));
/// [number-smoothing]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        demo_ui(ctx, enforce_validity(ctx, get_state(ctx, 42)));
    });
}

static demo the_demo("number-smoothing", init_demo);

} // namespace number_smoothing

namespace color_smoothing {

void
demo_ui(dom::context ctx, duplex<rgb8> color)
{
    // clang-format off
/// [color-smoothing]
dom::colored_box(
    ctx, smooth(ctx, color, animated_transition{ease_in_out_curve, 200}));
dom::button(ctx, "Go Light", color <<= rgb8(210, 210, 220));
dom::button(ctx, "Go Dark", color <<= rgb8(50, 50, 55));
/// [color-smoothing]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        demo_ui(ctx, get_state(ctx, rgb8(50, 50, 55)));
    });
}

static demo the_demo("color-smoothing", init_demo);

} // namespace color_smoothing

namespace flickering {

void
demo_ui(dom::context ctx, duplex<int> n)
{
    // clang-format off
/// [flickering]
dom::text(ctx, "Enter N:");
dom::input(ctx, n);
dom::button(ctx, "++", ++n);
dom::text(ctx, "Here's a flickering signal carrying N * 2:");
dom::text(ctx,
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
        demo_ui(ctx, enforce_validity(ctx, get_state(ctx, 1)));
    });
}

static demo the_demo("flickering-demo", init_demo);

} // namespace flickering

namespace deflickering {

void
demo_ui(dom::context ctx, duplex<int> n)
{
    // clang-format off
/// [deflickering]
dom::text(ctx, "Enter N:");
dom::input(ctx, n);
dom::button(ctx, "++", ++n);
dom::text(ctx, "Here's the same signal from above with deflickering:");
dom::text(ctx,
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
        demo_ui(ctx, get_state(ctx, 1));
    });
}

static demo the_demo("deflickering-demo", init_demo);

} // namespace deflickering
